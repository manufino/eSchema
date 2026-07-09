/*
 * MainWindow.cpp
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

// Construction/teardown, signal wiring, and the document lifecycle (New/
// Open/Save/Print/close). The rest of MainWindow's behaviour - too large to
// keep in one file - lives alongside this one: MainWindowLibraryPanel.cpp
// (the Libraries dock), MainWindowPropertiesPanel.cpp (the Properties dock)
// and MainWindowEditActions.cpp (Edit menu / selection actions). All four
// are still plain member function definitions of the same MainWindow class
// declared in MainWindow.h - just split across files for size, not split
// into separate classes.

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "LibraryManager.h"
#include "PrimitiveText.h"
#include "PrimitiveImage.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPainter>
#include <QFont>

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

    // Lets a dock area itself be split further (side by side, stacked, or a
    // mix), not just hold a single row/column of docks - without this, two
    // docks in the same area can only stack or tab, never sit side by side,
    // which is what "completely configurable, like professional drawing
    // apps" actually requires. AnimatedDocks (on by default) is deliberately
    // left out: it drives the docking preview through QWidgetAnimator, which
    // grabs a pixmap of the widget being dragged - reproducibly crashing in
    // Qt6Core.dll on this machine (an access violation confirmed via
    // Windows' own crash log, always at the same offset) as soon as a drag
    // starts, apparently a zero-sized grab hitting a bad code path in Qt's
    // ANGLE/GPU-accelerated pixmap backend. Docking/undocking/floating still
    // works exactly the same without the slide animation.
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    // The Libraries and Properties panels are QDockWidgets (freely movable/
    // floatable/dockable to any edge - or split side by side, or tabbed
    // together - matching the reference FidoCadJ editor's own dockable
    // panels), each exposing its own show/hide toggle here rather than the
    // single sidebar-toggle action that used to hide both at once when they
    // were still one fixed sidebar.
    ui->menuView->insertAction(ui->actionToolBarBaseVisible, ui->dockProperties->toggleViewAction());
    ui->menuView->insertAction(ui->actionToolBarBaseVisible, ui->dockLibraries->toggleViewAction());
    // Restores whatever dock/toolbar arrangement (position, size, floating,
    // tabbing) was in effect when the window was last closed - see
    // closeEvent(), which is what actually saves it. Silently does nothing
    // on the very first run (no saved state yet) or if the saved layout no
    // longer matches the current set of dock widgets, leaving the .ui's own
    // default arrangement in place.
    const QByteArray dockState = SettingsManager::getInstance().loadSetting("window_dock_state").toByteArray();
    if (!dockState.isEmpty())
        restoreState(dockState);

    layerToolBarWidget = new LayerToolBarWidget(this);
    ui->toolBarTools->addWidget(layerToolBarWidget); // add the layer combobox to the toolbar

    sheetScene = new Sheet();
    sheetScene->setSceneRect(0,0,5000,5000); // fix the scene dimensions

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
    connect(ui->graphicsView, &SheetView::zoomScaleIsChanged, ui->statusbar, &StatusBar::zoomLevel);
    connect(ui->actionAdjustView, &QAction::triggered, ui->graphicsView, &SheetView::adjustView);
    connect(ui->actionLayerManager, &QAction::triggered, this, &MainWindow::clickLayerManagerAction);
    connect(ui->actionShortcuts, &QAction::triggered, this, &MainWindow::clickShortcutsAction);
    connect(ui->actionMirror, &QAction::triggered, this, &MainWindow::clickMirrorAction);
    connect(ui->actionRotate, &QAction::triggered, this, &MainWindow::clickRotateAction);
    connect(ui->actionConvertMacroToPrimitives, &QAction::triggered, this, &MainWindow::clickConvertMacroToPrimitivesAction);
    connect(ui->actionCreateMacro, &QAction::triggered, this, &MainWindow::clickCreateMacroAction);
    // Keeps every selection/clipboard-dependent Edit action's enabled state
    // current - both for the menu bar and the right-click context menu,
    // which reuses these same QAction objects (see showCanvasContextMenu()).
    connect(sheetScene, &QGraphicsScene::selectionChanged, this, &MainWindow::updateEditActionsState);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateEditActionsState);
    updateEditActionsState();

    connect(ui->graphicsView, &SheetView::contextMenuRequested, this, &MainWindow::showCanvasContextMenu);

    // Proprietà panel: reflects the selection, and each field applies its
    // edit back to every selected primitive when the user actually changes
    // it (editingFinished()/activated()-style signals, not textChanged() -
    // so partially-typed text or a value the sync itself just set doesn't
    // get pushed back out on every keystroke).
    connect(sheetScene, &QGraphicsScene::selectionChanged, this, &MainWindow::updatePropertiesPanel);
    updatePropertiesPanel();
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, [this]() {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder())
            primitive->setName(ui->lineEdit->text());
    });
    connect(ui->lineEdit_2, &QLineEdit::editingFinished, this, [this]() {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder())
            primitive->setValue(ui->lineEdit_2->text());
    });
    connect(ui->cbPropLayer, &LayerComboBox::layerSelectedChanged, this, [this]() {
        Layer *layer = ui->cbPropLayer->selectedLayer();
        if (!layer)
            return;
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder())
            primitive->setLayer(layer);
    });
    connect(ui->checkBox, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder())
            primitive->setIsFilled(checked);
    });
    connect(ui->cbPropLineStyle, &QComboBox::activated, this, [this]() {
        const Qt::PenStyle style = ui->cbPropLineStyle->currentPenStyle();
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder())
            primitive->penStyleIsChanged(style);
    });
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text)
                static_cast<PrimitiveText *>(primitive)->setFontName(font.family());
        }
    });
    connect(ui->spinOpacity, &QSpinBox::valueChanged, this, [this](int percent) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Image)
                static_cast<PrimitiveImage *>(primitive)->setOpacity(percent / 100.0);
        }
    });
    connect(ui->checkBoxKeepAspectRatio, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Image)
                static_cast<PrimitiveImage *>(primitive)->setKeepAspectRatio(checked);
        }
    });
    connect(ui->checkBoxGrayscale, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Image)
                static_cast<PrimitiveImage *>(primitive)->setGrayscale(checked);
        }
    });

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
    if (confirmDiscardChanges()) {
        // Persists the current dock/toolbar arrangement (Libraries/Properties
        // panel position, size, floating, tabbing) so it's restored exactly
        // as left on the next launch - see the constructor's restoreState().
        SettingsManager::getInstance().saveSetting("window_dock_state", saveState());
        event->accept();
    } else {
        event->ignore();
    }
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
