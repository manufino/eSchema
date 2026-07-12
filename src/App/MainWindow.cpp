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
#include "DxfReader.h"
#include "DxfWriter.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "LibraryManager.h"
#include "PrimitiveText.h"
#include "PrimitiveImage.h"
#include "PrimitivePad.h"
#include "PrimitivePcbTrack.h"
#include "PrimitiveComplexCurve.h"
#include "ThemeManager.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QGuiApplication>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPainter>
#include <QFont>
#include <QSignalBlocker>
#include <QSvgGenerator>
#include <QInputDialog>
#include <QImage>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Idempotent - a headless conversion run before the GUI (the -c option
    // without -n) has already populated them.
    LayerList::getInstance().createDefaultLayers();

    ui->setupUi(this);
    updateWindowTitle();

    // main.cpp's startup call to ThemeManager::apply() runs before this
    // window (and therefore its actions/icons) exist, so re-apply here to
    // pick up the dark theme's white-tinted icons on the very first show.
    ThemeManager::apply();

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

    ui->rulerVertical->setOrientation(Qt::Vertical);

    // Dropping a .fcd/.dxf file anywhere on the window opens/imports it (see
    // dragEnterEvent()/dropEvent()). The canvas must not accept drops itself:
    // QGraphicsView forwards drag events to the scene, which accepts them by
    // default, swallowing any drop over the drawing area before it could
    // propagate up to this window's handler.
    setAcceptDrops(true);
    ui->graphicsView->setAcceptDrops(false);

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
    // Same live-settings pattern as line_width above, for the connection-dot
    // size every PrimitiveConnection now reads live via effectiveDiameter()
    // instead of caching its own copy.
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged, this, [this]() {
        const qreal value = SettingsManager::getInstance().loadSetting("connection_diameter").toDouble();
        sheetScene->setConnectionDiameter(value > 0 ? value : 2.0);
        sheetScene->update();
    });

    // Reflects whatever was last saved (or the compiled-in default, for a
    // settings file saved before this action persisted anything) rather than
    // always starting checked. Signals blocked: this is loading the saved
    // state, not the user changing it, so it must not immediately re-trigger
    // a save (same pattern as StatusBar's grid/snap toggle buttons).
    const QVariant rulersVisible = SettingsManager::getInstance().loadSetting("rulers_visible");
    const QSignalBlocker blockRulersAction(ui->actionRulersVisible);
    ui->actionRulersVisible->setChecked(rulersVisible.isValid() ? rulersVisible.toBool() : true);

    setConnections();
    updateRecentFilesMenu();
    updateRulers();
    updateRulersVisibility();
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
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->graphicsView, &SheetView::zoomScaleIsChanged, ui->statusbar, &StatusBar::zoomLevel);
    connect(ui->graphicsView, &SheetView::viewTransformChanged, this, &MainWindow::updateRulers);
    connect(ui->graphicsView, &SheetView::mouseMoved, this, [this](QPointF scenePos) {
        ui->rulerHorizontal->setMarkerPosition(scenePos.x(), true);
        ui->rulerVertical->setMarkerPosition(scenePos.y(), true);
    });
    connect(ui->graphicsView, &SheetView::mouseLeftView, this, [this]() {
        ui->rulerHorizontal->setMarkerPosition(0, false);
        ui->rulerVertical->setMarkerPosition(0, false);
    });
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &MainWindow::updateRulersVisibility);
    // Persists across restarts (see the constructor's own read of the same
    // key) - toggled() only fires on an actual state change, so the initial
    // setChecked() there can't be relied on to also save it back.
    connect(ui->actionRulersVisible, &QAction::toggled, this, [](bool checked) {
        SettingsManager::getInstance().saveSetting("rulers_visible", checked);
    });
    connect(sheetScene, &Sheet::primitivesChanged, this, [this]() {
        int macroCount = 0;
        for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib)
                ++macroCount;
        }
        ui->statusbar->primitiveCounts(sheetScene->primitives().size(), macroCount);
    });
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
    connect(ui->checkBoxArrowStart, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows())
                primitive->setArrowAtStart(checked);
        }
    });
    connect(ui->checkBoxArrowEnd, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows())
                primitive->setArrowAtEnd(checked);
        }
    });
    connect(ui->checkBoxArrowEmpty, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows())
                primitive->setArrowStyleEmpty(checked);
        }
    });
    connect(ui->checkBoxArrowLimiter, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows())
                primitive->setArrowStyleLimiter(checked);
        }
    });
    connect(ui->spinArrowLength, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows())
                primitive->setArrowLength(value);
        }
    });
    connect(ui->spinArrowHalfWidth, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows())
                primitive->setArrowHalfWidth(value);
        }
    });
    connect(ui->checkBoxCurveClosed, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Spline)
                static_cast<PrimitiveComplexCurve *>(primitive)->setClosed(checked);
        }
    });
    connect(ui->spinTrackWidth, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::PcbTrack)
                static_cast<PrimitivePcbTrack *>(primitive)->setWidth(value);
        }
    });
    connect(ui->spinPadWidth, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Pad) {
                auto *pad = static_cast<PrimitivePad *>(primitive);
                pad->setOuterSize(value, pad->outerHeight());
            }
        }
    });
    connect(ui->spinPadHeight, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Pad) {
                auto *pad = static_cast<PrimitivePad *>(primitive);
                pad->setOuterSize(pad->outerWidth(), value);
            }
        }
    });
    connect(ui->spinPadHole, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Pad)
                static_cast<PrimitivePad *>(primitive)->setHoleDiameter(value);
        }
    });
    connect(ui->cbPadStyle, &QComboBox::activated, this, [this](int index) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Pad)
                static_cast<PrimitivePad *>(primitive)->setStyle(static_cast<PrimitivePad::Style>(index));
        }
    });
    // The text content itself (which may carry the ^/_ superscript/subscript
    // markup rendered by DecoratedText) - editable after placement, like
    // every other text property.
    connect(ui->lineEditTextContent, &QLineEdit::editingFinished, this, [this]() {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text)
                static_cast<PrimitiveText *>(primitive)->setText(ui->lineEditTextContent->text());
        }
    });
    connect(ui->spinTextSizeX, &QSpinBox::valueChanged, this, [this](int value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
                auto *text = static_cast<PrimitiveText *>(primitive);
                text->setSize(text->sizeY(), value);
            }
        }
    });
    connect(ui->spinTextSizeY, &QSpinBox::valueChanged, this, [this](int value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
                auto *text = static_cast<PrimitiveText *>(primitive);
                text->setSize(value, text->sizeX());
            }
        }
    });
    connect(ui->spinTextOrientation, &QSpinBox::valueChanged, this, [this](int value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text)
                static_cast<PrimitiveText *>(primitive)->setOrientationDeg(value);
        }
    });
    // Bold/Italic/Mirrored share one styleFlags() bitmask - each checkbox
    // flips just its own bit, keeping the other two untouched.
    connect(ui->checkBoxTextBold, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
                auto *text = static_cast<PrimitiveText *>(primitive);
                int flags = text->styleFlags();
                flags = checked ? (flags | PrimitiveText::Bold) : (flags & ~PrimitiveText::Bold);
                text->setStyleFlags(flags);
            }
        }
    });
    connect(ui->checkBoxTextItalic, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
                auto *text = static_cast<PrimitiveText *>(primitive);
                int flags = text->styleFlags();
                flags = checked ? (flags | PrimitiveText::Italic) : (flags & ~PrimitiveText::Italic);
                text->setStyleFlags(flags);
            }
        }
    });
    connect(ui->checkBoxTextMirrored, &QCheckBox::toggled, this, [this](bool checked) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
                auto *text = static_cast<PrimitiveText *>(primitive);
                int flags = text->styleFlags();
                flags = checked ? (flags | PrimitiveText::Mirrored) : (flags & ~PrimitiveText::Mirrored);
                text->setStyleFlags(flags);
            }
        }
    });

    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::clickDeleteAction);
    connect(ui->actionCut, &QAction::triggered, this, &MainWindow::clickCutAction);
    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::clickCopyAction);
    connect(ui->actionCopyAsImage, &QAction::triggered, this, &MainWindow::clickCopyAsImageAction);
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
    connect(ui->actionImportDxf, &QAction::triggered, this, &MainWindow::clickImportDxfAction);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::clickSaveAction);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::clickSaveAsAction);
    connect(ui->actionPrint, &QAction::triggered, this, &MainWindow::clickPrintAction);
    connect(ui->actionExportPdf, &QAction::triggered, this, &MainWindow::clickExportPdfAction);
    connect(ui->actionExportSvg, &QAction::triggered, this, &MainWindow::clickExportSvgAction);
    connect(ui->actionExportPng, &QAction::triggered, this, &MainWindow::clickExportPngAction);
    connect(ui->actionExportDxf, &QAction::triggered, this, &MainWindow::clickExportDxfAction);
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

    // Alt+arrows nudge the selection by one snap step (see nudgeSelection()).
    // Window-scoped shortcuts, so they work with any part of the UI focused;
    // typing in a text field is unaffected (Alt+arrows isn't a text key).
    const struct { QKeySequence keys; QPointF direction; } nudges[] = {
        { QKeySequence(Qt::ALT | Qt::Key_Left),  QPointF(-1, 0) },
        { QKeySequence(Qt::ALT | Qt::Key_Right), QPointF(1, 0) },
        { QKeySequence(Qt::ALT | Qt::Key_Up),    QPointF(0, -1) },
        { QKeySequence(Qt::ALT | Qt::Key_Down),  QPointF(0, 1) },
    };
    for (const auto &nudge : nudges) {
        auto *shortcut = new QShortcut(nudge.keys, this);
        const QPointF direction = nudge.direction;
        connect(shortcut, &QShortcut::activated, this, [this, direction]() {
            nudgeSelection(direction);
        });
    }
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
    setWindowTitle(QString("  eSchema  [ Ver. ") + APP_VERSION + QString(" ]  -  ") + name);
}

void MainWindow::setCurrentFilePath(const QString &filePath)
{
    currentFilePath = filePath;
    updateWindowTitle();
    // Every path that becomes "the current document" (Open, Save As, a file
    // passed on the command line) is by definition the most recently used one.
    if (!filePath.isEmpty())
        addToRecentFiles(filePath);
}

void MainWindow::addToRecentFiles(const QString &filePath)
{
    const QString absolute = QFileInfo(filePath).absoluteFilePath();
    QStringList recents = SettingsManager::getInstance().loadSetting("recent_files").toStringList();
    // Case-insensitive de-duplication: on Windows the same file can arrive
    // with different casing (file dialog vs command line vs drag&drop).
    for (int i = recents.size() - 1; i >= 0; --i) {
        if (QString::compare(recents.at(i), absolute, Qt::CaseInsensitive) == 0)
            recents.removeAt(i);
    }
    recents.prepend(absolute);
    while (recents.size() > 10)
        recents.removeLast();
    SettingsManager::getInstance().saveSetting("recent_files", recents);
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    ui->menuRecentFiles->clear();
    const QStringList recents = SettingsManager::getInstance().loadSetting("recent_files").toStringList();
    ui->menuRecentFiles->setEnabled(!recents.isEmpty());

    // Every handler below rebuilds this menu (directly or via openFile() ->
    // setCurrentFilePath() -> addToRecentFiles()), and rebuilding deletes the
    // very action whose triggered() is being emitted - queued connections
    // defer the handler until the emission stack has unwound, making that
    // deletion safe.
    for (const QString &path : recents) {
        QAction *action = ui->menuRecentFiles->addAction(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            if (!QFileInfo::exists(path)) {
                QMessageBox::warning(this, tr("Apri recenti"),
                                      tr("Il file non esiste piu':\n%1").arg(path));
                QStringList recents = SettingsManager::getInstance().loadSetting("recent_files").toStringList();
                recents.removeAll(path);
                SettingsManager::getInstance().saveSetting("recent_files", recents);
                updateRecentFilesMenu();
                return;
            }
            if (!confirmDiscardChanges())
                return;
            openFile(path);
        }, Qt::QueuedConnection);
    }

    ui->menuRecentFiles->addSeparator();
    QAction *clearAction = ui->menuRecentFiles->addAction(tr("Svuota elenco"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        SettingsManager::getInstance().saveSetting("recent_files", QStringList());
        updateRecentFilesMenu();
    }, Qt::QueuedConnection);
}

void MainWindow::updateRulers()
{
    // QGraphicsView::mapFromScene() returns coordinates relative to its
    // *viewport* child widget, not the graphicsView widget itself - the two
    // differ by the view's frame width. The rulers are separate sibling
    // widgets in the same layout column/row as graphicsView (not children of
    // its viewport), so placing ticks straight from mapFromScene() was off
    // by that frame width, making a ruler's numbers land next to the wrong
    // point of the drawing (mismatched against the status bar's readout of
    // the same screen position). Routing through global screen coordinates
    // sidesteps whatever the actual offset between the two widgets is.
    const QPoint viewportOriginGlobal = ui->graphicsView->viewport()->mapToGlobal(
                ui->graphicsView->mapFromScene(QPointF(0, 0)));
    const QPoint originInHorizontalRuler = ui->rulerHorizontal->mapFromGlobal(viewportOriginGlobal);
    const QPoint originInVerticalRuler = ui->rulerVertical->mapFromGlobal(viewportOriginGlobal);

    const QTransform transform = ui->graphicsView->transform();
    ui->rulerHorizontal->setViewTransform(originInHorizontalRuler.x(), transform.m11());
    ui->rulerVertical->setViewTransform(originInVerticalRuler.y(), transform.m22());

    const qreal minorStep = ui->graphicsView->minorGridStep();
    const qreal majorStep = ui->graphicsView->majorGridStep();
    ui->rulerHorizontal->setGridSteps(minorStep, majorStep);
    ui->rulerVertical->setGridSteps(minorStep, majorStep);
}

void MainWindow::updateRulersVisibility()
{
    const QVariant val = SettingsManager::getInstance().loadSetting("rulers_visible");
    const bool visible = val.isValid() ? val.toBool() : true;
    ui->rulerHorizontal->setVisible(visible);
    ui->rulerVertical->setVisible(visible);
    ui->rulerCorner->setVisible(visible);
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
    if (!confirmDiscardChanges())
        return;

    sheetScene->clearPrimitives();
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(QString());
}

void MainWindow::clickOpenAction()
{
    if (!confirmDiscardChanges())
        return;

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

void MainWindow::clickImportDxfAction()
{
    if (!confirmDiscardChanges())
        return;

    const QString path = QFileDialog::getOpenFileName(this, tr("Importa da DXF"), QString(),
                                                        tr("DXF (*.dxf)"));
    if (path.isEmpty())
        return;

    importDxfFile(path);
}

bool MainWindow::importDxfFile(const QString &filePath)
{
    // Same non-undoable bulk-load contract as Open/FidoCadReader::read() -
    // this replaces the current sheet's contents entirely, matching this
    // app's single-always-open-document model rather than merging into it.
    QString error;
    QStringList warnings;
    if (!DxfReader::readFile(filePath, sheetScene, &error, &warnings)) {
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile aprire il file:\n%1").arg(error));
        return false;
    }
    sheetScene->undoStack()->setClean();
    // A DXF file has no notion of "this eSchema document's own path" - it's
    // an import, not a native Open, so the next Save still goes through Save
    // As rather than silently overwriting the .dxf with .fcd content.
    setCurrentFilePath(QString());

    if (!warnings.isEmpty()) {
        QMessageBox::information(this, tr("Importa da DXF"),
                                  tr("Alcuni elementi del file DXF non sono stati importati:\n\n%1")
                                          .arg(warnings.join(QLatin1Char('\n'))));
    }
    return true;
}

QString MainWindow::droppableFilePath(const QMimeData *mimeData)
{
    if (!mimeData->hasUrls())
        return QString();
    // Only the first droppable file counts - this is a single-document app,
    // so a multi-file drop can't open more than one anyway.
    for (const QUrl &url : mimeData->urls()) {
        const QString path = url.toLocalFile();
        if (path.endsWith(QStringLiteral(".fcd"), Qt::CaseInsensitive)
                || path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
            return path;
    }
    return QString();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!droppableFilePath(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QString path = droppableFilePath(event->mimeData());
    if (path.isEmpty())
        return;

    event->acceptProposedAction();
    if (!confirmDiscardChanges())
        return;

    if (path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
        importDxfFile(path);
    else
        openFile(path);
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

void MainWindow::clickExportPdfAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Esporta PDF"), tr("Il disegno e' vuoto."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Esporta come PDF"), QString(),
                                                  tr("File PDF (*.pdf)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        path += QStringLiteral(".pdf");

    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    // Reuses the same fit-to-page-and-center logic as File > Stampa
    // (renderForPrint()) - a PDF page is rendered exactly like a printed one,
    // just written straight to a file instead of a physical/virtual printer.
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    renderForPrint(&printer);

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);
}

void MainWindow::clickExportSvgAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Esporta SVG"), tr("Il disegno e' vuoto."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Esporta come SVG"), QString(),
                                                  tr("File SVG (*.svg)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive))
        path += QStringLiteral(".svg");

    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    const QRectF sourceRect = sheetScene->itemsBoundingRect();
    const QRectF targetRect(QPointF(0, 0), sourceRect.size());

    // Scene units map 1:1 to SVG user units - the drawing is exported at its
    // native scale, giving a resolution-independent vector file that a
    // viewer/editor can scale to whatever size is needed, unlike the fixed
    // pixel grid a PNG export bakes in.
    QSvgGenerator generator;
    generator.setFileName(path);
    generator.setSize(targetRect.size().toSize());
    generator.setViewBox(targetRect);
    generator.setTitle(tr("Schema eSchema"));

    QPainter painter(&generator);
    sheetScene->render(&painter, targetRect, sourceRect);
    painter.end();

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);
}

void MainWindow::clickExportPngAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Esporta PNG"), tr("Il disegno e' vuoto."));
        return;
    }

    bool ok = false;
    const int scale = QInputDialog::getInt(this, tr("Esporta come PNG"),
                                            tr("Fattore di scala (pixel per unita' di disegno):"),
                                            4, 1, 20, 1, &ok);
    if (!ok)
        return;

    QString path = QFileDialog::getSaveFileName(this, tr("Esporta come PNG"), QString(),
                                                  tr("File PNG (*.png)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive))
        path += QStringLiteral(".png");

    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    const QRectF sourceRect = sheetScene->itemsBoundingRect();
    const QSize targetSize = (sourceRect.size() * scale).toSize();

    // The scene itself paints no background (that's SheetView::drawBackground(),
    // an on-screen-only grid/paper visual aid - see Sheet::drawForeground() for
    // the one thing the scene does paint outside of primitives), so fill white
    // explicitly here or the exported PNG would come out with a transparent
    // background instead of a plain page like the PDF/print output.
    QImage image(targetSize, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    sheetScene->render(&painter, QRectF(QPointF(0, 0), targetSize), sourceRect);
    painter.end();

    if (!image.save(path))
        QMessageBox::warning(this, tr("Esporta PNG"), tr("Impossibile salvare il file PNG."));

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);
}

void MainWindow::clickExportDxfAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Esporta DXF"), tr("Il disegno e' vuoto."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, tr("Esporta come DXF"), QString(),
                                                  tr("File DXF (*.dxf)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
        path += QStringLiteral(".dxf");

    QString error;
    if (!DxfWriter::writeFile(sheetScene, path, &error))
        QMessageBox::warning(this, tr("Errore"), tr("Impossibile salvare il file:\n%1").arg(error));
}
