/*
 * mainwindow.cpp
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "MirrorPrimitiveCommand.h"
#include "RotatePrimitiveCommand.h"
#include "DeletePrimitiveCommand.h"
#include "CreatePrimitiveCommand.h"
#include "MovePrimitiveCommand.h"
#include "LibraryManager.h"
#include "PrimitiveMacro.h"
#include "DialogCreateMacro.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPainter>
#include <QTreeWidget>
#include <QFont>
#include <algorithm>
#include <limits>

namespace {
// FidoCadJ's 16 default layers and colors (FIDOSPECS.md 3.1). Populating all
// of them (rather than just a single master layer) at startup matters for
// FidoCadJ round-trip fidelity: opening a file that references layer index 2
// must land on that layer, not silently collapse to layer 0 for want of one
// existing.
void createDefaultLayers()
{
    static const QColor colors[16] = {
        QColor(0, 0, 0),        QColor(0, 0, 128),      QColor(255, 0, 0),      QColor(0, 128, 128),
        QColor(255, 200, 0),    QColor(127, 255, 0),    QColor(0, 255, 255),    QColor(0, 128, 0),
        QColor(154, 205, 50),   QColor(255, 20, 147),   QColor(181, 155, 12),   QColor(1, 128, 255),
        QColor(225, 225, 225, 242), QColor(162, 162, 162, 230), QColor(95, 95, 95, 230), QColor(0, 0, 0)
    };
    for (int i = 0; i < 16; ++i)
        LayerList::getInstance().addLayer(new Layer(QString("Layer %1").arg(i), colors[i], i == 0));
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    createDefaultLayers();

    ui->setupUi(this);
    updateWindowTitle();

    layerToolBarWidget = new LayerToolBarWidget(this);
    ui->toolBarTools->addWidget(layerToolBarWidget); // aggiungo il layer combobox alla toolbar

    sheetScene = new Sheet();
    sheetScene->setSceneRect(0,0,5000,5000); // fisso le dimensioni della scena

    ui->graphicsView->setScene(sheetScene);

    placementController = new PrimitivePlacementController(ui->graphicsView, sheetScene,
                                                             ui->toolBarPrimitive, ui->cbPropLayer,
                                                             ui->checkBox, this);
    ui->graphicsView->setPlacementController(placementController);

    selectionHandleController = new SelectionHandleController(sheetScene, this);

    LibraryManager::getInstance().loadLibraries();
    buildLibraryPanel();
    // Rebuilds the panel (cheap: icons are cached per key+size in
    // LibraryManager, so only an actual macro_icon_size change re-renders
    // anything) whenever any option changes - matches SheetView's own
    // settingChanged() convention of reacting broadly rather than filtering
    // for the one setting it cares about.
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &MainWindow::buildLibraryPanel);
    // Same rebuild whenever LibraryManager's own data changes (e.g. a new
    // user macro was just saved via "Crea macro dalla selezione").
    connect(&LibraryManager::getInstance(), &LibraryManager::librariesReloaded,
            this, &MainWindow::buildLibraryPanel);
    // Syncs the open document's own line width too, not just new ones -
    // every primitive already reads Sheet::lineWidth() live at paint time
    // (GraphicsPrimitive::effectiveLineWidth()), so this takes effect
    // instantly, matching the reference FidoCadJ editor's own single shared
    // Globals.lineWidth (there is no isolated "per-document" copy there
    // either - loading a file with its own "FJC A" mutates that very same
    // global for the rest of the session).
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged, this, [this]() {
        const qreal value = SettingsManager::getInstance().loadSetting("line_width").toDouble();
        sheetScene->setLineWidth(value > 0 ? value : 0.5);
        sheetScene->update();
    });

    setConnections();
}

MainWindow::~MainWindow()
{
    Utils::instance().DeleteSafely(sheetScene);
    Utils::instance().DeleteSafely(layerToolBarWidget);
    Utils::instance().DeleteSafely(ui);
}

void MainWindow::setConnections()
{
    connect(ui->graphicsView, &SheetView::mouseMoved, ui->statusbar, &StatusBar::sceneMousePos);
    connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::clickOptionAction);
    connect(ui->actionInformation, &QAction::triggered, this, &MainWindow::clickAboutAction);
    connect(ui->statusbar->btnGrid, &QPushButton::toggled, ui->graphicsView, &SheetView::enableGrid);
    connect(ui->graphicsView, &SheetView::zoomScaleIsChanged, ui->statusbar, &StatusBar::zoomLevel);
    connect(ui->actionAdjustView, &QAction::triggered, ui->graphicsView, &SheetView::adjustView);
    connect(ui->actionLayerManager, &QAction::triggered, this, &MainWindow::clickLayerManagerAction);
    connect(ui->actionShortcuts, &QAction::triggered, this, &MainWindow::clickShortcutsAction);
    connect(ui->DSpinBoxLineHeight, &QSpinBox::valueChanged, ui->cbPropLineStyle, &PenStyleComboBox::lineWidthChanged);
    connect(ui->actionMirror, &QAction::triggered, this, &MainWindow::clickMirrorAction);
    connect(ui->actionRotate, &QAction::triggered, this, &MainWindow::clickRotateAction);
    connect(ui->actionConvertMacroToPrimitives, &QAction::triggered, this, &MainWindow::clickConvertMacroToPrimitivesAction);
    connect(ui->actionCreateMacro, &QAction::triggered, this, &MainWindow::clickCreateMacroAction);
    // Only meaningful with something selected - matches the request that
    // this specific action (unlike the rest of the Edit menu, which just
    // silently no-ops on an empty selection) actually reflect that state.
    connect(sheetScene, &QGraphicsScene::selectionChanged, this, [this]() {
        ui->actionCreateMacro->setEnabled(!sheetScene->selectedItems().isEmpty());
    });
    ui->actionCreateMacro->setEnabled(false);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::clickDeleteAction);
    connect(ui->actionCut, &QAction::triggered, this, &MainWindow::clickCutAction);
    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::clickCopyAction);
    connect(ui->actionPaste, &QAction::triggered, this, &MainWindow::clickPasteAction);
    connect(ui->actionDuplicate, &QAction::triggered, this, &MainWindow::clickDuplicateAction);
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::clickSelectAllAction);
    connect(ui->actionAlignLeft, &QAction::triggered, this, &MainWindow::clickAlignLeftAction);
    connect(ui->actionAlignRight, &QAction::triggered, this, &MainWindow::clickAlignRightAction);
    connect(ui->actionAlignTop, &QAction::triggered, this, &MainWindow::clickAlignTopAction);
    connect(ui->actionAlignBottom, &QAction::triggered, this, &MainWindow::clickAlignBottomAction);
    connect(ui->actionAlignCenterHorizontal, &QAction::triggered, this, &MainWindow::clickAlignCenterHorizontalAction);
    connect(ui->actionAlignCenterVertical, &QAction::triggered, this, &MainWindow::clickAlignCenterVerticalAction);
    connect(ui->actionDistributeHorizontal, &QAction::triggered, this, &MainWindow::clickDistributeHorizontalAction);
    connect(ui->actionDistributeVertical, &QAction::triggered, this, &MainWindow::clickDistributeVerticalAction);
    connect(ui->actionNewDraw, &QAction::triggered, this, &MainWindow::clickNewAction);
    connect(ui->actionOpenFile, &QAction::triggered, this, &MainWindow::clickOpenAction);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::clickSaveAction);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::clickSaveAsAction);
    connect(ui->actionPrint, &QAction::triggered, this, &MainWindow::clickPrintAction);
    // QWidget::close() reaches closeEvent(), which does the unsaved-changes
    // check - so File > Close and the window's own titlebar X button behave
    // identically instead of needing separate logic.
    connect(ui->actionClose, &QAction::triggered, this, &QWidget::close);

    QUndoStack *undo = sheetScene->undoStack();
    connect(ui->actionUndo, &QAction::triggered, undo, &QUndoStack::undo);
    connect(ui->actionRestore, &QAction::triggered, undo, &QUndoStack::redo);
    connect(undo, &QUndoStack::canUndoChanged, ui->actionUndo, &QAction::setEnabled);
    connect(undo, &QUndoStack::canRedoChanged, ui->actionRestore, &QAction::setEnabled);
    connect(undo, &QUndoStack::undoTextChanged, this, [this](const QString &text) {
        ui->actionUndo->setText(text.isEmpty() ? tr("Annulla") : tr("Annulla: %1").arg(text));
    });
    connect(undo, &QUndoStack::redoTextChanged, this, [this](const QString &text) {
        ui->actionRestore->setText(text.isEmpty() ? tr("Ripristina") : tr("Ripristina: %1").arg(text));
    });
    ui->actionUndo->setEnabled(undo->canUndo());
    ui->actionRestore->setEnabled(undo->canRedo());

    connect(ui->txtSearch, &QLineEdit::textChanged, this, &MainWindow::filterLibraryPanel);
}

void MainWindow::buildLibraryPanel()
{
    while (ui->toolBoxLib->count() > 0) {
        QWidget *page = ui->toolBoxLib->widget(0);
        ui->toolBoxLib->removeItem(0);
        delete page;
    }

    // Falls back to the compiled-in default (matches SettingsManager::
    // restoreDefaultSettings()) rather than the 0 QSettings::value() would
    // return for a settings file saved before this option existed.
    int iconSize = SettingsManager::getInstance().loadSetting("macro_icon_size").toInt();
    if (iconSize <= 0)
        iconSize = 32;
    const QSize iconQSize(iconSize, iconSize);

    for (const MacroLibrary &library : LibraryManager::getInstance().libraries()) {
        // A tree (library page -> category node -> macro leaf) instead of a
        // flat icon list, so a library's categories - the second grouping
        // level the .fcl format itself defines via "{...}" lines - actually
        // read as sections instead of being just another same-sized cell in
        // an icon grid.
        auto *tree = new QTreeWidget(ui->toolBoxLib);
        tree->setHeaderHidden(true);
        tree->setIconSize(iconQSize);
        tree->setIndentation(12);
        // NOT uniform: category rows (text-only) and macro rows (icon-sized,
        // which the user can set anywhere from 16 to 128px) are genuinely
        // different heights - forcing a single shared height was clipping/
        // overlapping the icons once they got larger than whatever height
        // Qt had cached from the first (short, text-only) row it measured.

        for (const QString &category : library.categoryOrder) {
            auto *categoryItem = new QTreeWidgetItem(tree, QStringList(category));
            categoryItem->setFlags(Qt::ItemIsEnabled);
            QFont categoryFont = categoryItem->font(0);
            categoryFont.setBold(true);
            categoryItem->setFont(0, categoryFont);
            // Collapsed by default: several libraries here have 15-24
            // categories (PCB Footprints, IHRAM, Simboli Elettrotecnica) -
            // expanding all of them at once overflows the panel so badly
            // that only the first one or two categories are ever visible,
            // making a perfectly complete library look empty/broken.

            for (const MacroDescriptor &descriptor : library.macrosByCategory.value(category)) {
                auto *item = new QTreeWidgetItem(categoryItem, QStringList(descriptor.name));
                item->setIcon(0, LibraryManager::getInstance().icon(descriptor.key, iconSize));
                item->setData(0, Qt::UserRole, descriptor.key);
                item->setToolTip(0, descriptor.name);
                // An explicit size hint, not just the icon size passed to the
                // tree above: some styles' default item delegate doesn't
                // reliably grow the row to fit a large icon on its own.
                item->setSizeHint(0, QSize(iconSize + 24, iconSize + 4));
            }
        }

        connect(tree, &QTreeWidget::itemClicked, this, &MainWindow::clickLibraryMacroItem);

        const QString label = !library.displayName.isEmpty() ? library.displayName
                : (library.filename.isEmpty() ? tr("Standard") : library.filename);
        ui->toolBoxLib->addItem(tree, label);
    }
}

void MainWindow::filterLibraryPanel(const QString &text)
{
    for (int page = 0; page < ui->toolBoxLib->count(); ++page) {
        auto *tree = qobject_cast<QTreeWidget *>(ui->toolBoxLib->widget(page));
        if (!tree)
            continue;

        bool pageHasMatch = false;
        for (int c = 0; c < tree->topLevelItemCount(); ++c) {
            QTreeWidgetItem *categoryItem = tree->topLevelItem(c);
            bool categoryHasMatch = false;
            for (int m = 0; m < categoryItem->childCount(); ++m) {
                QTreeWidgetItem *macroItem = categoryItem->child(m);
                const bool matches = text.isEmpty() || macroItem->text(0).contains(text, Qt::CaseInsensitive);
                macroItem->setHidden(!matches);
                categoryHasMatch = categoryHasMatch || matches;
            }
            categoryItem->setHidden(!categoryHasMatch);
            // Categories are collapsed by default (see buildLibraryPanel()) -
            // while actively searching, expand every category with a match
            // so its children are actually visible instead of just present-
            // but-collapsed; collapse back down once the search is cleared.
            categoryItem->setExpanded(!text.isEmpty() && categoryHasMatch);
            pageHasMatch = pageHasMatch || categoryHasMatch;
        }
        if (!text.isEmpty() && pageHasMatch)
            ui->toolBoxLib->setCurrentIndex(page); // jump to the first library with a match
    }
}

void MainWindow::clickLibraryMacroItem(QTreeWidgetItem *item)
{
    const QString key = item->data(0, Qt::UserRole).toString();
    if (!key.isEmpty())
        placementController->armMacroPlacement(key);
}

void MainWindow::clickOptionAction()
{
    optionDialog = new DialogOptions(this);
    connect(optionDialog, &QDialog::finished, optionDialog, &QObject::deleteLater);
    optionDialog->show();
}

void MainWindow::clickAboutAction()
{
    aboutDialog = new DialogAbout(this);
    connect(aboutDialog, &QDialog::finished, aboutDialog, &QObject::deleteLater);
    aboutDialog->show();
}

void MainWindow::clickShortcutsAction()
{
    shortcutsDialog = new DialogShortcuts(this);
    connect(shortcutsDialog, &QDialog::finished, shortcutsDialog, &QObject::deleteLater);
    shortcutsDialog->show();
}

void MainWindow::clickLayerManagerAction()
{
    layerManager = new DialogLayerList(this);
    connect(layerManager, &QDialog::finished, layerManager, &QObject::deleteLater);
    layerManager->show();
}

// Mirror/Rotate/Delete/SelectAll all operate on the current scene selection.
// Mirror and Rotate pivot around controlPoint(0) of the first selected
// primitive in document order - matching the reference FidoCadJ editor
// (EditorActions.rotateAllSelected/mirrorAllSelected pivot on
// getFirstSelectedPrimitive().getFirstPoint()), rather than e.g. the
// selection's bounding-box center.
GraphicsPrimitive *MainWindow::firstSelectedPrimitive() const
{
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->isSelected())
            return primitive;
    }
    return nullptr;
}

// Mirror/Rotate/Delete are never applied directly here - each pushes an undo
// command instead, whose redo() (auto-invoked once by QUndoStack::push()) is
// the sole place the actual mutation happens. Multiple selected primitives
// are wrapped in a macro so they undo/redo together as one step.
void MainWindow::clickMirrorAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Specchia"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new MirrorPrimitiveCommand(primitive, Qt::Horizontal, pivot));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickRotateAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Ruota"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new RotatePrimitiveCommand(primitive, pivot));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickConvertMacroToPrimitivesAction()
{
    QList<PrimitiveMacro *> macros;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        if (auto *macro = dynamic_cast<PrimitiveMacro *>(primitive))
            macros.append(macro);
    }
    if (macros.isEmpty())
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    // Always grouped, even for a single macro: converting one already means
    // one delete plus N creates, and all of it must undo as one step.
    undo->beginMacro(tr("Converti macro in primitive"));
    for (PrimitiveMacro *macro : macros) {
        const QList<GraphicsPrimitive *> expanded = macro->convertToPrimitives(sheetScene);
        for (GraphicsPrimitive *primitive : expanded) {
            // push() calls redo() synchronously, which is what actually adds
            // the primitive to the scene - only safe to select it afterwards.
            undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
            primitive->setSelected(true);
        }
        undo->push(new DeletePrimitiveCommand(sheetScene, macro));
    }
    undo->endMacro();
}

void MainWindow::clickCreateMacroAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    DialogCreateMacro dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    // A fresh, independent parse of the selection (never the live canvas
    // primitives themselves) so its control points can be freely offset
    // below without touching what's actually on screen - same round-trip
    // Duplicate/Paste already rely on.
    const QString serialized = FidoCadWriter::writeSelection(selected);
    QList<GraphicsPrimitive *> clones = FidoCadReader::parse(serialized, sheetScene);
    if (clones.isEmpty())
        return;

    // Anchors the macro at the selection's top-left corner: every clone is
    // shifted so that corner lands exactly on the library format's
    // conventional (100,100) macro origin, matching how every shipped
    // library symbol is itself laid out.
    QRectF bounds;
    bool first = true;
    for (GraphicsPrimitive *clone : clones) {
        for (int i = 0; i < clone->controlPointCount(); ++i) {
            const QPointF point = clone->controlPoint(i);
            if (first) {
                bounds = QRectF(point, QSizeF(0, 0));
                first = false;
            } else {
                bounds = bounds.united(QRectF(point, QSizeF(0, 0)));
            }
        }
    }
    const QPointF offset = QPointF(100, 100) - bounds.topLeft();
    for (GraphicsPrimitive *clone : clones)
        clone->translateControlPoints(offset);
    const QString body = FidoCadWriter::writeSelection(clones);
    qDeleteAll(clones);

    QString errorMessage;
    const bool ok = LibraryManager::getInstance().addUserMacro(
                dialog.libraryFilename(), dialog.libraryDisplayName(),
                dialog.key(), dialog.macroName(), dialog.category(), body, &errorMessage);
    if (!ok)
        QMessageBox::warning(this, tr("Crea macro"), errorMessage);
}

void MainWindow::clickDeleteAction()
{
    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Elimina"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickSelectAllAction()
{
    for (QGraphicsItem *item : sheetScene->items())
        item->setSelected(true);
}

// Applies each primitive's computed delta as an undoable MovePrimitiveCommand
// (skipping primitives whose delta is a no-op), matching the reference
// FidoCadJ editor's align/distribute (EditorActions.java), which likewise
// moves each affected primitive and saves one undo state per operation.
void MainWindow::moveSelectedPrimitives(const QHash<GraphicsPrimitive *, QPointF> &deltas, const QString &undoLabel)
{
    QList<GraphicsPrimitive *> toMove;
    for (auto it = deltas.constBegin(); it != deltas.constEnd(); ++it) {
        if (!it.value().isNull())
            toMove.append(it.key());
    }
    if (toMove.isEmpty())
        return;

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = toMove.size() > 1;
    if (multiple)
        undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : toMove) {
        const QVector<QPointF> before = primitive->controlPointSnapshot();
        primitive->translateControlPoints(deltas.value(primitive));
        undo->push(new MovePrimitiveCommand(primitive, before, primitive->controlPointSnapshot()));
    }
    if (multiple)
        undo->endMacro();
}

// Align/distribute operate on each primitive's own bounding box (in scene
// coordinates, since pos() is always pinned at the origin - see
// GraphicsPrimitive's header comment), mirroring the reference FidoCadJ
// editor's per-primitive getPosition()/getSize() - but computed as a proper
// min/max reduction rather than replicating a latent bug in FidoCadJ's own
// getSize() that mishandles primitives with more than 2 control points
// (polygons, complex curves, macros).
void MainWindow::clickAlignLeftAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal leftmost = std::numeric_limits<qreal>::max();
    for (GraphicsPrimitive *primitive : selected)
        leftmost = qMin(leftmost, primitive->boundingRect().left());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(leftmost - primitive->boundingRect().left(), 0));
    moveSelectedPrimitives(deltas, tr("Allinea a sinistra"));
}

void MainWindow::clickAlignRightAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal rightmost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected)
        rightmost = qMax(rightmost, primitive->boundingRect().right());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(rightmost - primitive->boundingRect().right(), 0));
    moveSelectedPrimitives(deltas, tr("Allinea a destra"));
}

void MainWindow::clickAlignTopAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal topmost = std::numeric_limits<qreal>::max();
    for (GraphicsPrimitive *primitive : selected)
        topmost = qMin(topmost, primitive->boundingRect().top());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, topmost - primitive->boundingRect().top()));
    moveSelectedPrimitives(deltas, tr("Allinea in alto"));
}

void MainWindow::clickAlignBottomAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal bottommost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected)
        bottommost = qMax(bottommost, primitive->boundingRect().bottom());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, bottommost - primitive->boundingRect().bottom()));
    moveSelectedPrimitives(deltas, tr("Allinea in basso"));
}

// Aligns every selected primitive's own vertical center to a shared
// horizontal line (the selection's overall vertical midpoint) - named after
// FidoCadJ's alignHorizontalCenterSelected, which centers things relative to
// a horizontal centerline (what other tools often call "align middle").
void MainWindow::clickAlignCenterHorizontalAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal topmost = std::numeric_limits<qreal>::max();
    qreal bottommost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected) {
        const QRectF rect = primitive->boundingRect();
        topmost = qMin(topmost, rect.top());
        bottommost = qMax(bottommost, rect.bottom());
    }
    const qreal verticalCenter = (topmost + bottommost) / 2.0;

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, verticalCenter - primitive->boundingRect().center().y()));
    moveSelectedPrimitives(deltas, tr("Allinea al centro orizzontale"));
}

// Aligns every selected primitive's own horizontal center to a shared
// vertical line - FidoCadJ's alignVerticalCenterSelected ("align center" in
// more common CAD/DTP terminology).
void MainWindow::clickAlignCenterVerticalAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal leftmost = std::numeric_limits<qreal>::max();
    qreal rightmost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected) {
        const QRectF rect = primitive->boundingRect();
        leftmost = qMin(leftmost, rect.left());
        rightmost = qMax(rightmost, rect.right());
    }
    const qreal horizontalCenter = (leftmost + rightmost) / 2.0;

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(horizontalCenter - primitive->boundingRect().center().x(), 0));
    moveSelectedPrimitives(deltas, tr("Allinea al centro verticale"));
}

// Distributes selected primitives evenly by their left edges, keeping the
// leftmost and rightmost primitives fixed - matching FidoCadJ's
// distributeHorizontallySelected() exactly, including its >=3 requirement
// (with only 2 selected there's nothing "in between" to redistribute).
void MainWindow::clickDistributeHorizontalAction()
{
    QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 3)
        return;

    std::sort(selected.begin(), selected.end(), [](GraphicsPrimitive *a, GraphicsPrimitive *b) {
        return a->boundingRect().left() < b->boundingRect().left();
    });

    const qreal leftmostX = selected.first()->boundingRect().left();
    const qreal rightmostX = selected.last()->boundingRect().left();
    const qreal spacing = (rightmostX - leftmostX) / (selected.size() - 1);

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (int i = 1; i < selected.size() - 1; ++i) {
        GraphicsPrimitive *primitive = selected.at(i);
        const qreal targetX = leftmostX + i * spacing;
        deltas.insert(primitive, QPointF(targetX - primitive->boundingRect().left(), 0));
    }
    moveSelectedPrimitives(deltas, tr("Distribuisci orizzontalmente"));
}

void MainWindow::clickDistributeVerticalAction()
{
    QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 3)
        return;

    std::sort(selected.begin(), selected.end(), [](GraphicsPrimitive *a, GraphicsPrimitive *b) {
        return a->boundingRect().top() < b->boundingRect().top();
    });

    const qreal topmostY = selected.first()->boundingRect().top();
    const qreal bottommostY = selected.last()->boundingRect().top();
    const qreal spacing = (bottommostY - topmostY) / (selected.size() - 1);

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (int i = 1; i < selected.size() - 1; ++i) {
        GraphicsPrimitive *primitive = selected.at(i);
        const qreal targetY = topmostY + i * spacing;
        deltas.insert(primitive, QPointF(0, targetY - primitive->boundingRect().top()));
    }
    moveSelectedPrimitives(deltas, tr("Distribuisci verticalmente"));
}

QList<GraphicsPrimitive *> MainWindow::selectedPrimitivesInOrder() const
{
    QList<GraphicsPrimitive *> result;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->isSelected())
            result.append(primitive);
    }
    return result;
}

// Copy/Cut put the selection on the system clipboard as FidoCadJ text
// (FidoCadWriter::writeSelection()) - matching the reference FidoCadJ editor,
// which copies as text rather than an app-private format, so a paste can also
// land in a plain text editor (and, conversely, so hand-written FCD snippets
// can be pasted straight into the drawing).
void MainWindow::clickCopyAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(selected));
}

void MainWindow::clickCutAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(selected));

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Taglia"));
    for (GraphicsPrimitive *primitive : selected)
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    if (multiple)
        undo->endMacro();
}

// Shared by Paste (parses clipboard text) and Duplicate (parses a freshly
// serialized copy of the current selection without touching the system
// clipboard at all - matching the visible result of the reference FidoCadJ
// editor's Duplicate, which is copySelected()+paste() under the hood, but
// without the side effect of clobbering whatever the user last copied).
void MainWindow::pasteFromText(const QString &text, const QString &undoLabel)
{
    const QList<GraphicsPrimitive *> pasted = FidoCadReader::parse(text, sheetScene);
    if (pasted.isEmpty())
        return;

    // Offset by one grid step and select the result, matching the reference
    // FidoCadJ editor's paste (CopyPasteActions.paste() shifts by the current
    // grid step; PopUpMenu's Paste handler passes that grid step in) - so the
    // pasted copy is visibly distinct from what was copied and immediately
    // ready to drag into place.
    const QVariant stepVal = SettingsManager::getInstance().loadSetting("snap_step");
    const int step = stepVal.isValid() && stepVal.toInt() > 0 ? stepVal.toInt() : 10;
    const QPointF offset(step, step);

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = pasted.size() > 1;
    if (multiple)
        undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : pasted) {
        primitive->translateControlPoints(offset);
        // push() calls redo() synchronously, which is what actually adds the
        // primitive to the scene - only safe to select it afterwards.
        undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
        primitive->setSelected(true);
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickPasteAction()
{
    const QString text = QGuiApplication::clipboard()->text();
    if (text.isEmpty())
        return;
    pasteFromText(text, tr("Incolla"));
}

void MainWindow::clickDuplicateAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    pasteFromText(FidoCadWriter::writeSelection(selected), tr("Duplica"));
}

void MainWindow::updateWindowTitle()
{
    const QString name = currentFilePath.isEmpty()
            ? tr("Nuovo disegno* (non salvato)")
            : QFileInfo(currentFilePath).fileName();
    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" BETA ]  -  ") + name);
}

void MainWindow::setCurrentFilePath(const QString &filePath)
{
    currentFilePath = filePath;
    updateWindowTitle();
}

bool MainWindow::saveToPath(const QString &filePath)
{
    QString error;
    if (!FidoCadWriter::writeFile(sheetScene, filePath, &error)) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile salvare il file:\n%1").arg(error));
        return false;
    }
    // Marks the undo stack's current position as "the saved state" - isClean()
    // (used by confirmDiscardChanges()) reports true again until the next
    // change, rather than staying permanently "dirty" after the first edit.
    sheetScene->undoStack()->setClean();
    return true;
}

// If there are unsaved changes, asks whether to save them before whatever the
// caller is about to do (close the app, discard the document for New/Open).
// Returns false only when the user cancels - callers must abort in that case.
bool MainWindow::confirmDiscardChanges()
{
    if (sheetScene->undoStack()->isClean())
        return true;

    const auto answer = QMessageBox::question(
                this, tr("Modifiche non salvate"),
                tr("Ci sono modifiche non salvate. Vuoi salvarle?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);

    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Discard)
        return true;

    clickSaveAction();
    // clickSaveAction() falls back to Save As for a never-saved document; if
    // the user then cancels that dialog, the stack is still dirty - treat
    // that the same as cancelling here, rather than closing/discarding anyway.
    return sheetScene->undoStack()->isClean();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges())
        event->accept();
    else
        event->ignore();
}

void MainWindow::clickNewAction()
{
    sheetScene->clearPrimitives();
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(QString());
}

void MainWindow::clickOpenAction()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Apri disegno"), QString(),
                                                        tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;

    openFile(path);
}

bool MainWindow::openFile(const QString &filePath)
{
    QString error;
    if (!FidoCadReader::readFile(filePath, sheetScene, &error)) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile aprire il file:\n%1").arg(error));
        return false;
    }
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(filePath);
    return true;
}

void MainWindow::clickSaveAction()
{
    if (currentFilePath.isEmpty()) {
        clickSaveAsAction();
        return;
    }
    saveToPath(currentFilePath);
}

void MainWindow::clickSaveAsAction()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Salva disegno con nome"), QString(),
                                                 tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".fcd"), Qt::CaseInsensitive))
        path += QStringLiteral(".fcd");

    if (saveToPath(path))
        setCurrentFilePath(path);
}

void MainWindow::clickPrintAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Stampa"), tr("Il disegno e' vuoto."));
        return;
    }

    // Selection highlighting (the selected-primitive green blend) and resize
    // handles are editor chrome, not part of the drawing - hide them for the
    // whole preview/print session by clearing the selection up front (rather
    // than per-repaint in renderForPrint(), which would flicker the handles
    // in the editor view every time the preview repaints) and restore it once
    // the dialog closes.
    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle(tr("Anteprima di stampa"));
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &MainWindow::renderForPrint);
    preview.exec();

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);
}

void MainWindow::renderForPrint(QPrinter *printer)
{
    const QRectF sourceRect = sheetScene->itemsBoundingRect();
    if (sourceRect.isEmpty())
        return;

    const QRect targetRect = printer->pageLayout().paintRectPixels(printer->resolution());

    // Fit the drawing within the printable area, preserving aspect ratio and
    // centering it - a schematic's shape rarely matches the page's, and
    // stretching it unevenly would distort right angles and text.
    const qreal scale = qMin(targetRect.width() / sourceRect.width(),
                              targetRect.height() / sourceRect.height());
    const QSizeF scaledSize = sourceRect.size() * scale;
    const QRectF centeredTarget(targetRect.left() + (targetRect.width() - scaledSize.width()) / 2.0,
                                 targetRect.top() + (targetRect.height() - scaledSize.height()) / 2.0,
                                 scaledSize.width(), scaledSize.height());

    QPainter painter(printer);
    sheetScene->render(&painter, centeredTarget, sourceRect);
}
