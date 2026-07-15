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
#include "DialogAttachImage.h"
#include "DialogExport.h"
#include "DialogPrintOptions.h"
#include "DxfReader.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "GraphicExporter.h"
#include "LibraryManager.h"
#include "PrimitiveText.h"
#include "PrimitiveImage.h"
#include "PrimitivePad.h"
#include "PrimitivePcbTrack.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitiveLine.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "ThemeManager.h"
#include "UpdateChecker.h"
#include "CreatePrimitiveCommand.h"
#include "DeletePrimitiveCommand.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
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
#include <QShortcut>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QUndoCommand>
#include <QPainterPath>

namespace {
// Passed to save/restoreState() so a layout saved by an older build - with a
// different set of dock widgets/toolbars than the current one - is cleanly
// rejected by Qt (restoreState() returns false and leaves the .ui's own
// default arrangement in place) instead of being force-applied onto a
// mismatched widget set, which could leave the saved QDockAreaLayoutInfo for
// one area (e.g. Left) internally inconsistent - it looked exactly like
// "docking to that one side is just broken" until the stale saved state
// was cleared by hand. Bump this whenever a dock widget or toolbar is
// added/removed/renamed.
constexpr int DockStateVersion = 3; // 2: added the Modify toolbar; 3: its default row moved below the main toolbar

// No bundled icon reads as "snap" the way a magnet does - a plain grid
// glyph (the obvious alternative) would sit right next to actionShowGrid's
// own near-identical grid icon and be just as easy to mix up as the two
// status-bar buttons this action replaces used to be. Rendered at runtime
// the same way the layer lock icons are (see LayerIcons.cpp) rather than
// adding a one-off PNG asset for a single toolbar action.
QIcon renderMagnetIcon(int size = 24)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal scale = size / 24.0;
    painter.scale(scale, scale);

    // Horseshoe body: two legs joined by a rounded top arc.
    QPainterPath body;
    body.moveTo(6.5, 20.0);
    body.lineTo(6.5, 10.0);
    body.arcTo(QRectF(6.5, 2.0, 11.0, 16.0), 180.0, -180.0);
    body.lineTo(17.5, 20.0);

    QPen bodyPen(QColor(110, 110, 115), 4.2, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
    painter.setPen(bodyPen);
    painter.drawPath(body);

    // The classic red/silver pole banding is what reads as "magnet" at a
    // glance.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(196, 58, 48));
    painter.drawRoundedRect(QRectF(4.0, 17.3, 5.0, 4.4), 1.0, 1.0);
    painter.setBrush(QColor(180, 180, 185));
    painter.drawRoundedRect(QRectF(15.0, 17.3, 5.0, 4.4), 1.0, 1.0);

    painter.end();
    return QIcon(pixmap);
}
}

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
    ui->menuView->insertAction(ui->actionToolBarBaseVisible, ui->dockFcdCode->toggleViewAction());
    // Restores whatever dock/toolbar arrangement (position, size, floating,
    // tabbing) was in effect when the window was last closed - see
    // closeEvent(), which is what actually saves it. Silently does nothing
    // on the very first run (no saved state yet) or if the saved layout no
    // longer matches the current set of dock widgets, leaving the .ui's own
    // default arrangement in place.
    const QByteArray dockState = SettingsManager::getInstance().loadSetting("window_dock_state").toByteArray();
    if (!dockState.isEmpty())
        restoreState(dockState, DockStateVersion);

    // Must run *after* restoreState(): corner ownership is itself part of
    // the state blob QMainWindow::saveState() writes, so restoring an old
    // one (saved before this fix, or just Qt's own compiled-in default)
    // would silently overwrite whatever was set above. Qt's own default
    // hands both top corners to Qt::TopDockWidgetArea and both bottom
    // corners to Qt::BottomDockWidgetArea - since this app has no dock
    // widgets there (only the horizontal toolbar), that pinched the left
    // edge's actual drop zone thin right where users naturally try to drag a
    // panel (just below the toolbar), reading as "docking only works on the
    // right". Explicitly giving each side's corners to that same side fixes
    // it symmetrically for both left and right.
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

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
    // a save (same pattern as the grid/snap toolbar actions right below).
    const QVariant rulersVisible = SettingsManager::getInstance().loadSetting("rulers_visible");
    const QSignalBlocker blockRulersAction(ui->actionRulersVisible);
    ui->actionRulersVisible->setChecked(rulersVisible.isValid() ? rulersVisible.toBool() : true);

    // No bundled iconset for this one - see renderMagnetIcon()'s own comment.
    ui->actionSnapToGrid->setIcon(renderMagnetIcon());

    const QVariant gridVisible = SettingsManager::getInstance().loadSetting("grid_visible");
    const QSignalBlocker blockGridAction(ui->actionShowGrid);
    ui->actionShowGrid->setChecked(gridVisible.isValid() ? gridVisible.toBool() : true);
    connect(ui->actionShowGrid, &QAction::toggled, this, [](bool checked) {
        SettingsManager::getInstance().saveSetting("grid_visible", checked);
    });

    // Same "snap_enabled" key the Options dialog's own checkbox reads/writes
    // (GlobalUtils.h's snapToGrid() is what actually consults it) - this
    // toolbar button is just a quicker way to flip the very same setting.
    const QVariant snapEnabled = SettingsManager::getInstance().loadSetting("snap_enabled");
    const QSignalBlocker blockSnapAction(ui->actionSnapToGrid);
    ui->actionSnapToGrid->setChecked(snapEnabled.isValid() ? snapEnabled.toBool() : true);
    connect(ui->actionSnapToGrid, &QAction::toggled, this, [](bool checked) {
        SettingsManager::getInstance().saveSetting("snap_enabled", checked);
    });

    setConnections();
    updateRecentFilesMenu();
    updateRulers();
    updateRulersVisibility();

    autosaveTimer = new QTimer(this);
    connect(autosaveTimer, &QTimer::timeout, this, &MainWindow::autosaveTick);
    restartAutosaveTimer();
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &MainWindow::restartAutosaveTimer);

    updateChecker = new UpdateChecker(this);
    connect(updateChecker, &UpdateChecker::updateAvailable, this, &MainWindow::handleUpdateAvailable);
    connect(updateChecker, &UpdateChecker::upToDate, this, &MainWindow::handleUpdateUpToDate);
    connect(updateChecker, &UpdateChecker::checkFailed, this, &MainWindow::handleUpdateCheckFailed);
    // Silent unless an update is actually found (see handleUpdateUpToDate()/
    // handleUpdateCheckFailed()) - manualUpdateCheck defaults to false, so
    // this startup check reads as "automatic" to both of those.
    if (SettingsManager::getInstance().loadSetting("check_updates_on_startup").toBool())
        updateChecker->checkForUpdates();

    // Must run before main.cpp's own w.openFile(commandLineFile) call, which
    // happens synchronously right after this constructor returns - so this
    // stays a plain synchronous call here rather than a deferred/queued one,
    // even though the window itself isn't shown yet at this point.
    checkForAutosaveRecovery();
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
    connect(ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::clickCheckUpdatesAction);
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
    connect(ui->actionZoomToSelection, &QAction::triggered, ui->graphicsView, &SheetView::adjustViewToSelection);
    connect(ui->actionLayerManager, &QAction::triggered, this, &MainWindow::clickLayerManagerAction);
    connect(ui->actionAttachImage, &QAction::triggered, this, &MainWindow::clickAttachImageAction);
    connect(ui->actionShortcuts, &QAction::triggered, this, &MainWindow::clickShortcutsAction);
    connect(ui->btnApplyFcdCode, &QPushButton::clicked, this, &MainWindow::clickApplyFcdCodeAction);
    connect(ui->btnRefreshFcdCode, &QPushButton::clicked, this, &MainWindow::clickRefreshFcdCodeAction);
    connect(ui->actionMirror, &QAction::triggered, this, &MainWindow::clickMirrorAction);
    connect(ui->actionRotate, &QAction::triggered, this, &MainWindow::clickRotateAction);
    connect(ui->actionBooleanUnion, &QAction::triggered, this, &MainWindow::clickBooleanUnionAction);
    connect(ui->actionBooleanSubtract, &QAction::triggered, this, &MainWindow::clickBooleanSubtractAction);
    connect(ui->actionBooleanIntersect, &QAction::triggered, this, &MainWindow::clickBooleanIntersectAction);
    // "Smooth results" is a persisted preference, not per-drawing state -
    // reloaded here and saved on every toggle.
    ui->actionBooleanSmooth->setChecked(
            SettingsManager::getInstance().loadSetting("boolean_smooth_results").toBool());
    connect(ui->actionBooleanSmooth, &QAction::toggled, this, [](bool on) {
        SettingsManager::getInstance().saveSetting("boolean_smooth_results", on);
    });
    connect(ui->actionRotateByAngle, &QAction::triggered, this, &MainWindow::clickRotateByAngleAction);
    connect(ui->actionScaleSelection, &QAction::triggered, this, &MainWindow::clickScaleSelectionAction);
    connect(ui->actionArray, &QAction::triggered, this, &MainWindow::clickArrayAction);
    connect(ui->actionConvertToPolygon, &QAction::triggered, this, &MainWindow::clickConvertToPolygonAction);
    connect(ui->actionConvertToCurve, &QAction::triggered, this, &MainWindow::clickConvertToCurveAction);
    connect(ui->actionSimplifyNodes, &QAction::triggered, this, &MainWindow::clickSimplifyNodesAction);
    connect(ui->actionFilletCorners, &QAction::triggered, this, &MainWindow::clickFilletCornersAction);
    connect(ui->actionChamferCorners, &QAction::triggered, this, &MainWindow::clickChamferCornersAction);
    connect(ui->actionConvertMacroToPrimitives, &QAction::triggered, this, &MainWindow::clickConvertMacroToPrimitivesAction);
    connect(ui->actionCreateMacro, &QAction::triggered, this, &MainWindow::clickCreateMacroAction);
    // Keeps every selection/clipboard-dependent Edit action's enabled state
    // current - both for the menu bar and the right-click context menu,
    // which reuses these same QAction objects (see showCanvasContextMenu()).
    connect(sheetScene, &QGraphicsScene::selectionChanged, this, &MainWindow::updateEditActionsState);
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateEditActionsState);
    updateEditActionsState();

    connect(ui->graphicsView, &SheetView::contextMenuRequested, this, &MainWindow::showCanvasContextMenu);

    // Properties panel: reflects the selection, and each field applies its
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
    connect(ui->spinLineLength, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Line)
                static_cast<PrimitiveLine *>(primitive)->setLength(value);
        }
    });
    connect(ui->spinShapeWidth, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Rectangle) {
                auto *rectangle = static_cast<PrimitiveRectangle *>(primitive);
                rectangle->setShapeSize(value, rectangle->shapeHeight());
            } else if (primitive->getPrimitiveType() == GraphicsPrimitive::Ellipse) {
                auto *ellipse = static_cast<PrimitiveEllipse *>(primitive);
                ellipse->setShapeSize(value, ellipse->shapeHeight());
            }
        }
    });
    connect(ui->spinShapeHeight, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::Rectangle) {
                auto *rectangle = static_cast<PrimitiveRectangle *>(primitive);
                rectangle->setShapeSize(rectangle->shapeWidth(), value);
            } else if (primitive->getPrimitiveType() == GraphicsPrimitive::Ellipse) {
                auto *ellipse = static_cast<PrimitiveEllipse *>(primitive);
                ellipse->setShapeSize(ellipse->shapeWidth(), value);
            }
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
    connect(ui->actionCopySplit, &QAction::triggered, this, &MainWindow::clickCopySplitAction);
    connect(ui->actionCopyAsImage, &QAction::triggered, this, &MainWindow::clickCopyAsImageAction);
    connect(ui->actionPaste, &QAction::triggered, this, &MainWindow::clickPasteAction);
    connect(ui->actionDuplicate, &QAction::triggered, this, &MainWindow::clickDuplicateAction);
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::clickSelectAllAction);
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::clickFindAction);
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
    connect(ui->actionSaveSplit, &QAction::triggered, this, &MainWindow::clickSaveSplitAction);
    connect(ui->actionPrint, &QAction::triggered, this, &MainWindow::clickPrintAction);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::clickExportAction);
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
        ui->actionUndo->setText(text.isEmpty() ? tr("Undo") : tr("Undo: %1").arg(text));
    });
    connect(undo, &QUndoStack::redoTextChanged, this, [this](const QString &text) {
        ui->actionRestore->setText(text.isEmpty() ? tr("Redo") : tr("Redo: %1").arg(text));
    });
    ui->actionUndo->setEnabled(undo->canUndo());
    ui->actionRestore->setEnabled(undo->canRedo());

    connect(undo, &QUndoStack::indexChanged, this, &MainWindow::refreshFcdCodeIfClean);
    // A drag-resize/move only lands on the undo stack at mouse release, and
    // never fires selectionChanged - without this the panel's new geometry
    // fields (length/width/height) would keep showing pre-drag values.
    connect(undo, &QUndoStack::indexChanged, this, &MainWindow::updatePropertiesPanel);
    syncFcdCodeFromSheet();

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

void MainWindow::clickCheckUpdatesAction()
{
    manualUpdateCheck = true;
    updateChecker->checkForUpdates();
}

void MainWindow::handleUpdateAvailable(const QString &version, const QUrl &releaseUrl)
{
    manualUpdateCheck = false;
    const auto answer = QMessageBox::question(
                this, tr("Update available"),
                tr("A new version of eSchema is available (%1).\nDo you want to download it now?").arg(version),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer == QMessageBox::Yes)
        QDesktopServices::openUrl(releaseUrl);
}

void MainWindow::handleUpdateUpToDate()
{
    if (manualUpdateCheck)
        QMessageBox::information(this, tr("Check for updates"),
                                  tr("eSchema is already up to date."));
    manualUpdateCheck = false;
}

void MainWindow::handleUpdateCheckFailed()
{
    // Silent when triggered by the startup check - no connection or a
    // firewall blocking it is completely normal and shouldn't nag the user
    // on every launch.
    if (manualUpdateCheck)
        QMessageBox::information(this, tr("Check for updates"),
                                  tr("Could not check for updates. Check your internet connection."));
    manualUpdateCheck = false;
}

void MainWindow::clickLayerManagerAction()
{
    layerManager = new DialogLayerList(this);
    connect(layerManager, &QDialog::finished, layerManager, &QObject::deleteLater);
    layerManager->show();
}

void MainWindow::clickAttachImageAction()
{
    DialogAttachImage dialog(sheetScene, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    if (dialog.imageRemoved()) {
        if (!sheetScene->hasBackgroundImage())
            return; // nothing to do
        sheetScene->clearBackgroundImage();
    } else {
        if (dialog.mimeSubtype().isEmpty() || dialog.base64Data().isEmpty())
            return; // OK'd without ever loading an image - a no-op
        sheetScene->setBackgroundImage(dialog.mimeSubtype(), dialog.base64Data(),
                                        dialog.resolution(), dialog.corner());
    }

    // Not itself undoable, matching every other document-config setting
    // (connection diameter, line width) this mirrors - just marks the
    // document dirty so the change actually gets offered a save, the same
    // trick checkForAutosaveRecovery() uses for a recovered drawing.
    sheetScene->undoStack()->push(new QUndoCommand(tr("Attach tracing image")));
}

void MainWindow::updateWindowTitle()
{
    const QString name = currentFilePath.isEmpty()
            ? tr("New drawing* (unsaved)")
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
                QMessageBox::warning(this, tr("Open recent"),
                                      tr("The file no longer exists:\n%1").arg(path));
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
    QAction *clearAction = ui->menuRecentFiles->addAction(tr("Clear list"));
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
        QMessageBox::warning(this, tr("Error"), tr("Unable to save the file:\n%1").arg(error));
        return false;
    }
    // Marks the undo stack's current position as "the saved state" - isClean()
    // (used by confirmDiscardChanges()) reports true again until the next
    // change, rather than staying permanently "dirty" after the first edit.
    sheetScene->undoStack()->setClean();
    // The drawing is now safely on disk at filePath - any outstanding
    // recovery sidecar for it is redundant.
    clearAutosave();
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
                this, tr("Unsaved changes"),
                tr("There are unsaved changes. Do you want to save them?"),
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
        SettingsManager::getInstance().saveSetting("window_dock_state", saveState(DockStateVersion));
        // A clean, deliberate exit - nothing left to recover next launch.
        clearAutosave();
        event->accept();
    } else {
        event->ignore();
    }
}

QString MainWindow::autosaveTargetPath() const
{
    if (!currentFilePath.isEmpty()) {
        const QFileInfo info(currentFilePath);
        return info.absolutePath() + QLatin1Char('/') + info.completeBaseName()
                + QStringLiteral(".autosave.fcd");
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/untitled.autosave.fcd");
}

void MainWindow::clearAutosave()
{
    const QString path = SettingsManager::getInstance().loadSetting("pending_autosave_path").toString();
    if (!path.isEmpty())
        QFile::remove(path);
    SettingsManager::getInstance().saveSetting("pending_autosave_path", QString());
    SettingsManager::getInstance().saveSetting("pending_autosave_original", QString());
}

void MainWindow::restartAutosaveTimer()
{
    const QVariant enabledVal = SettingsManager::getInstance().loadSetting("autosave_enabled");
    const bool enabled = enabledVal.isValid() ? enabledVal.toBool() : true;
    const QVariant intervalVal = SettingsManager::getInstance().loadSetting("autosave_interval_minutes");
    const int minutes = intervalVal.toInt() > 0 ? intervalVal.toInt() : 5;

    if (enabled)
        autosaveTimer->start(minutes * 60 * 1000);
    else
        autosaveTimer->stop();
}

void MainWindow::autosaveTick()
{
    // A clean document is already safe - either never edited, or its last
    // edit is already on disk at currentFilePath - nothing to recover.
    if (sheetScene->undoStack()->isClean())
        return;

    const QString path = autosaveTargetPath();
    QString error;
    // Best-effort: a failed autosave (e.g. a read-only directory) shouldn't
    // interrupt editing with an error dialog every few minutes.
    if (!FidoCadWriter::writeFile(sheetScene, path, &error))
        return;

    SettingsManager::getInstance().saveSetting("pending_autosave_path", path);
    SettingsManager::getInstance().saveSetting("pending_autosave_original", currentFilePath);
}

void MainWindow::checkForAutosaveRecovery()
{
    const QString path = SettingsManager::getInstance().loadSetting("pending_autosave_path").toString();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        // A stale pointer (e.g. the sidecar was deleted by hand) - forget it
        // rather than asking about a file that doesn't exist.
        clearAutosave();
        return;
    }

    const QString original = SettingsManager::getInstance().loadSetting("pending_autosave_original").toString();
    const QString description = original.isEmpty() ? tr("an untitled drawing")
                                                     : QFileInfo(original).fileName();
    const auto answer = QMessageBox::question(
                this, tr("Autosave recovery"),
                tr("eSchema wasn't closed properly last time.\n"
                   "An autosave was found for %1.\n\n"
                   "Do you want to recover it?").arg(description),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (answer != QMessageBox::Yes) {
        clearAutosave();
        return;
    }

    QString error;
    if (!FidoCadReader::readFile(path, sheetScene, &error)) {
        QMessageBox::warning(this, tr("Error"),
                              tr("Could not recover the autosave:\n%1").arg(error));
        clearAutosave();
        return;
    }

    if (!original.isEmpty())
        setCurrentFilePath(original);
    // Recovered content was never actually written to the real file (if
    // any) - readFile()'s bulk load leaves the stack clean like a normal
    // Open, so push a no-op command purely to flip isClean() to false.
    sheetScene->undoStack()->push(new QUndoCommand(tr("Autosave recovery")));
    clearAutosave();
}

void MainWindow::clickNewAction()
{
    if (!confirmDiscardChanges())
        return;

    sheetScene->clearPrimitives();
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(QString());
    // confirmDiscardChanges() above already resolved (saved or discarded)
    // whatever the previous document's autosave might have been tracking.
    clearAutosave();
    // setClean() alone doesn't fire indexChanged() - the FCD code dock would
    // otherwise keep showing the previous document.
    syncFcdCodeFromSheet();
}

void MainWindow::clickOpenAction()
{
    if (!confirmDiscardChanges())
        return;

    const QString path = QFileDialog::getOpenFileName(this, tr("Open drawing"), QString(),
                                                        tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;

    openFile(path);
}

bool MainWindow::openFile(const QString &filePath)
{
    QString error;
    if (!FidoCadReader::readFile(filePath, sheetScene, &error)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to open the file:\n%1").arg(error));
        return false;
    }
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(filePath);
    // Whatever the previous document's autosave was tracking is moot now -
    // its callers already resolved it via confirmDiscardChanges().
    clearAutosave();
    syncFcdCodeFromSheet(); // setClean() alone doesn't fire indexChanged()
    return true;
}

void MainWindow::clickImportDxfAction()
{
    if (!confirmDiscardChanges())
        return;

    const QString path = QFileDialog::getOpenFileName(this, tr("Import from DXF"), QString(),
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
        QMessageBox::warning(this, tr("Error"), tr("Unable to open the file:\n%1").arg(error));
        return false;
    }
    sheetScene->undoStack()->setClean();
    // A DXF file has no notion of "this eSchema document's own path" - it's
    // an import, not a native Open, so the next Save still goes through Save
    // As rather than silently overwriting the .dxf with .fcd content.
    setCurrentFilePath(QString());
    clearAutosave();
    syncFcdCodeFromSheet(); // setClean() alone doesn't fire indexChanged()

    if (!warnings.isEmpty()) {
        QMessageBox::information(this, tr("Import from DXF"),
                                  tr("Some elements of the DXF file were not imported:\n\n%1")
                                          .arg(warnings.join(QLatin1Char('\n'))));
    }
    return true;
}

void MainWindow::syncFcdCodeFromSheet()
{
    // setPlainText() doesn't clear the modified flag on its own - block
    // signals so this programmatic change doesn't itself trigger anything
    // (e.g. a spell-checker/undo-stack hookup some style sheet might add).
    const QSignalBlocker blocker(ui->txtFcdCode);
    ui->txtFcdCode->setPlainText(FidoCadWriter::write(sheetScene));
    ui->txtFcdCode->document()->setModified(false);
}

void MainWindow::refreshFcdCodeIfClean()
{
    if (!ui->txtFcdCode->document()->isModified())
        syncFcdCodeFromSheet();
}

void MainWindow::clickRefreshFcdCodeAction()
{
    syncFcdCodeFromSheet();
}

void MainWindow::clickApplyFcdCodeAction()
{
    // FidoCadReader::parse() never fails (malformed/unrecognized lines are
    // just skipped per FIDOSPECS.md's robustness contract) - so there's
    // nothing to validate before replacing the drawing with the result.
    // Document-wide FJC config lines (connection diameter/line width/layer
    // locks) in the text are silently ignored here, same as parse() already
    // does for Paste - those have their own dedicated UI (Options, Layer
    // manager) rather than being round-tripped through this box.
    const QList<GraphicsPrimitive *> parsed = FidoCadReader::parse(ui->txtFcdCode->toPlainText(), sheetScene);

    // Copied, not a live reference: DeletePrimitiveCommand::redo() (from the
    // push() calls below) mutates sheetScene's own primitive list as it
    // goes, which would otherwise invalidate this loop mid-iteration.
    const QList<GraphicsPrimitive *> oldPrimitives = sheetScene->primitives();

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Edit FCD code"));
    for (GraphicsPrimitive *primitive : oldPrimitives)
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    for (GraphicsPrimitive *primitive : parsed) {
        undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
        primitive->setSelected(true);
    }
    undo->endMacro();

    // Resyncs with the canonical re-serialization of what was just applied
    // (e.g. reformatted numbers) rather than leaving the user's raw typed
    // text in place - endMacro() above already fired indexChanged(), but
    // refreshFcdCodeIfClean() would have skipped it since the text still
    // reads as modified at that point.
    syncFcdCodeFromSheet();
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
    QString path = QFileDialog::getSaveFileName(this, tr("Save drawing as"), QString(),
                                                 tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".fcd"), Qt::CaseInsensitive))
        path += QStringLiteral(".fcd");

    if (saveToPath(path))
        setCurrentFilePath(path);
}

// Always prompts for a path and never touches currentFilePath/the undo
// stack's clean state - unlike Save/Save As, this writes a throwaway
// alternate copy of the drawing, not a new "current file".
void MainWindow::clickSaveSplitAction()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save split as"), QString(),
                                                 tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QStringLiteral(".fcd"), Qt::CaseInsensitive))
        path += QStringLiteral(".fcd");

    QList<GraphicsPrimitive *> owned;
    const QList<GraphicsPrimitive *> expanded = expandMacrosOneLevel(sheetScene->primitives(), owned);
    QString error;
    const bool ok = FidoCadWriter::writeExpandedFile(sheetScene, expanded, path, &error);
    qDeleteAll(owned);

    if (!ok)
        QMessageBox::warning(this, tr("Error"), tr("Unable to save the file:\n%1").arg(error));
}

void MainWindow::clickPrintAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Print"), tr("The drawing is empty."));
        return;
    }

    DialogPrintOptions optionsDialog(this);
    if (optionsDialog.exec() != QDialog::Accepted)
        return;

    // Read once here and stashed in members for renderForPrint() (a slot
    // invoked once per repaint by QPrintPreviewDialog, with no way to pass
    // extra arguments through paintRequested()) to read back.
    m_printMirror = optionsDialog.mirror();
    m_printBlackWhite = optionsDialog.blackWhite();
    m_printOneLayer = optionsDialog.singleLayer();

    // Selection highlighting (the selected-primitive green blend) and resize
    // handles are editor chrome, not part of the drawing - hide them for the
    // whole preview/print session by clearing the selection up front (rather
    // than per-repaint in renderForPrint(), which would flicker the handles
    // in the editor view every time the preview repaints) and restore it once
    // the dialog closes.
    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(optionsDialog.landscape() ? QPageLayout::Landscape
                                                          : QPageLayout::Portrait);
    double marginTop, marginBottom, marginLeft, marginRight;
    optionsDialog.margins(&marginTop, &marginBottom, &marginLeft, &marginRight);
    // QPageLayout::Unit has no Centimeter value - convert from the dialog's
    // centimeters to millimeters.
    printer.setPageMargins(QMarginsF(marginLeft, marginTop, marginRight, marginBottom) * 10.0,
                            QPageLayout::Millimeter);

    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle(tr("Print preview"));
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &MainWindow::renderForPrint);
    preview.exec();

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);
}

void MainWindow::renderForPrint(QPrinter *printer)
{
    // "Print only this layer": hide every other layer's primitives for this
    // render pass, through the same primitive-owned visibility flag the CLI/
    // export split-layer paths already use (paint() early-returns on it -
    // independent of the real QGraphicsItem visibility layer hiding drives,
    // so it doesn't disturb the editor view underneath the preview dialog).
    const QList<GraphicsPrimitive *> primitives = sheetScene->primitives();
    if (m_printOneLayer) {
        for (GraphicsPrimitive *primitive : primitives)
            primitive->setVisible(primitive->layer() == m_printOneLayer);
    }

    // Black & white: temporarily paint every layer black - primitives read
    // their layer's color live at paint time (GraphicsPrimitive::drawColor()).
    QList<Layer *> *layers = LayerList::getInstance().getList();
    QHash<Layer *, QColor> savedColors;
    if (m_printBlackWhite) {
        for (Layer *layer : *layers) {
            savedColors.insert(layer, layer->color());
            layer->setColor(QColor(Qt::black));
        }
    }

    const QRectF sourceRect = sheetScene->itemsBoundingRect();
    if (!sourceRect.isEmpty()) {
        const QRect targetRect = printer->pageLayout().paintRectPixels(printer->resolution());

        // Fit the drawing within the printable area, preserving aspect ratio
        // and centering it - a schematic's shape rarely matches the page's,
        // and stretching it unevenly would distort right angles and text.
        const qreal scale = qMin(targetRect.width() / sourceRect.width(),
                                  targetRect.height() / sourceRect.height());
        const QSizeF scaledSize = sourceRect.size() * scale;
        const QRectF centeredTarget(targetRect.left() + (targetRect.width() - scaledSize.width()) / 2.0,
                                     targetRect.top() + (targetRect.height() - scaledSize.height()) / 2.0,
                                     scaledSize.width(), scaledSize.height());

        QPainter painter(printer);
        if (m_printMirror) {
            // Flips left-right around the printed area's own center, keeping
            // it in the same place on the page rather than jumping to the
            // opposite side - useful for iron-on toner-transfer PCB etching.
            painter.translate(centeredTarget.center());
            painter.scale(-1, 1);
            painter.translate(-centeredTarget.center());
        }
        sheetScene->render(&painter, centeredTarget, sourceRect);
    }

    if (m_printBlackWhite) {
        for (auto it = savedColors.constBegin(); it != savedColors.constEnd(); ++it)
            it.key()->setColor(it.value());
    }
    if (m_printOneLayer) {
        for (GraphicsPrimitive *primitive : primitives)
            primitive->setVisible(true);
    }
}

void MainWindow::clickExportAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Export"), tr("The drawing is empty."));
        return;
    }

    DialogExport dialog(sheetScene->itemsBoundingRect().size(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString path = QFileDialog::getSaveFileName(this, tr("Export drawing"), QString(),
                                                  dialog.fileFilter());
    if (path.isEmpty())
        return;
    const QString suffix = QLatin1Char('.') + dialog.defaultSuffix();
    if (!path.endsWith(suffix, Qt::CaseInsensitive))
        path += suffix;

    // Selection highlighting and resize handles are editor chrome, not part
    // of the drawing - hide them for the render by clearing the selection,
    // restored right after (same pattern as printing).
    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    QString error;
    const bool ok = GraphicExporter::exportSheet(sheetScene, path,
                                                  dialog.options(), &error);

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);

    if (!ok)
        QMessageBox::warning(this, tr("Error"),
                              tr("Could not export the file:\n%1").arg(error));
}
