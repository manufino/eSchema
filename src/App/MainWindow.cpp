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
#include "appversion.h"
#include "DialogCustomizeToolbars.h"
#include "CommandPalette.h"
#include "ui_MainWindow.h"
#include "DialogAttachImage.h"
#include "DialogSymbolWizard.h"
#include "ArrowStyleComboBox.h"
#include "DockTitleTab.h"
#include <QUndoGroup>
#include <QSet>
#include <QDockWidget>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QLineEdit>
#include "DialogExport.h"
#include "DialogPrintOptions.h"
#include "DxfReader.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "GraphicExporter.h"
#include "LibraryManager.h"
#include "PrimitiveText.h"
#include "PrimitiveImage.h"
#include "PrimitiveMacro.h"
#include "WelcomeWidget.h"
#include "DrawingThumbnails.h"
#include <QLabel>
#include "PrimitivePad.h"
#include "PrimitivePcbTrack.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitiveLine.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "ThemeManager.h"
#include <QTreeWidget>
#include "UpdateChecker.h"
#include "CreatePrimitiveCommand.h"
#include "DeletePrimitiveCommand.h"
#include <QFileDialog>
#include <QInputDialog>
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
#include <QTabBar>
#include <QWidgetAction>
#include <cmath>
#include <functional>

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
constexpr int DockStateVersion = 4; // 2: added the Modify toolbar; 3: its default row moved below the main toolbar; 4: panels tabbed on the right by default

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
    ui->menuView->insertAction(ui->actionToolBarBaseVisible, ui->dockLayers->toggleViewAction());
    // The toggle actions inherit each panel's icon (set in the .ui), so the
    // View menu matches the dock tab bar - see refreshDockTabIcons().
    ui->dockProperties->toggleViewAction()->setIcon(ui->dockProperties->windowIcon());
    ui->dockLibraries->toggleViewAction()->setIcon(ui->dockLibraries->windowIcon());
    ui->dockFcdCode->toggleViewAction()->setIcon(ui->dockFcdCode->windowIcon());
    ui->dockLayers->toggleViewAction()->setIcon(ui->dockLayers->windowIcon());
    // The Layers panel starts hidden: unlike the other three panels it is
    // an on-demand tool (Tools > Layer management / Ctrl+L shows it), and
    // hiding it BEFORE restoreState() means an older saved layout that
    // doesn't know this dock yet leaves it hidden instead of dropping it
    // docked-and-visible onto the right edge.
    ui->dockLayers->hide();
    // Restores whatever dock/toolbar arrangement (position, size, floating,
    // tabbing) was in effect when the window was last closed - see
    // closeEvent(), which is what actually saves it. On the very first run
    // (no saved state yet), or if the saved layout no longer matches the
    // current set of dock widgets, the three panels are instead stacked as
    // tabs on the right edge, Libraries in front.
    // Window size/position from the previous session - separate from the
    // dock state below: saveState() covers toolbars/docks only, never the
    // window's own geometry. Empty on the very first launch, which keeps
    // the .ui's default size.
    const QByteArray windowGeometry = SettingsManager::getInstance().loadSetting("window_geometry").toByteArray();
    if (!windowGeometry.isEmpty())
        restoreGeometry(windowGeometry);

    const QByteArray dockState = SettingsManager::getInstance().loadSetting("window_dock_state").toByteArray();
    bool dockStateRestored = false;
    if (!dockState.isEmpty())
        dockStateRestored = restoreState(dockState, DockStateVersion);
    if (!dockStateRestored) {
        ui->dockLibraries->setVisible(true);
        ui->dockProperties->setVisible(true);
        ui->dockFcdCode->setVisible(true);
        tabifyDockWidget(ui->dockLibraries, ui->dockProperties);
        tabifyDockWidget(ui->dockProperties, ui->dockFcdCode);
        // The Layers panel tabs with the others too (so showing it lands in
        // the same group), but stays hidden until asked for - see above.
        tabifyDockWidget(ui->dockFcdCode, ui->dockLayers);
        ui->dockLibraries->raise();
    }

    // QMainWindow's own dock tab bars show only each tab's text - there is
    // no public API for their icons (see refreshDockTabIcons()) - so re-sync
    // them whenever a dock is moved/re-tabbed. Deferred with a 0-timer: at
    // the moment these signals fire, the internal tab bar hasn't been
    // rebuilt yet.
    // The side panels join the same IDE-style docking as the document docks:
    // grouped ones drop their title bar (the tab is the handle + close
    // button), lone/floating ones keep it. Re-evaluate on every layout
    // change, and re-sync their (icon-less by default) tab bars.
    auto refreshPanelDocks = [this]() {
        QTimer::singleShot(0, this, [this]() {
            refreshDockTabIcons();
            setupDockTabBars();
            updateDockTitleBars();
        });
    };
    for (QDockWidget *dock : { ui->dockLibraries, ui->dockProperties, ui->dockFcdCode, ui->dockLayers }) {
        connect(dock, &QDockWidget::dockLocationChanged, this, refreshPanelDocks);
        connect(dock, &QDockWidget::topLevelChanged, this, refreshPanelDocks);
        connect(dock, &QDockWidget::visibilityChanged, this, refreshPanelDocks);
    }
    // Same GroupedDragging the document area uses, so dragging a panel tab
    // moves the whole group.
    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
    refreshDockTabIcons();

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

    // Picking any drawing tool puts the active document's welcome card away
    // BEFORE the first canvas click, so that click draws instead of being
    // spent dismissing the card.
    connect(ui->toolBarPrimitive, &QToolBar::actionTriggered, this, [this](QAction *) {
        if (m_activeDocument && m_activeDocument->welcome)
            m_activeDocument->welcome->dismiss();
    });

    // Dropping a .fcd/.dxf file anywhere on the window opens/imports it (see
    // dragEnterEvent()/dropEvent()). Each DocumentView's canvas refuses
    // drops itself (see createDocument()): QGraphicsView forwards drag
    // events to the scene, which accepts them by default, swallowing any
    // drop over the drawing area before it could propagate up here.
    setAcceptDrops(true);

    // --- Multi-document setup ---------------------------------------------
    m_undoGroup = new QUndoGroup(this);
    // The document area is a nested QMainWindow: each open drawing is a
    // QDockWidget inside it, so drawings can be tabbed together, split side
    // by side (vertically or horizontally) or floated, all with Qt's native
    // dock dragging. Nested + tabbed docks are what make the splits and the
    // default "all tabbed together" grouping possible.
    m_documentArea = new QMainWindow(this);
    m_documentArea->setDockOptions(QMainWindow::AllowNestedDocks
                                   | QMainWindow::AllowTabbedDocks
                                   | QMainWindow::GroupedDragging);
    m_documentArea->setDockNestingEnabled(true);
    m_documentArea->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    // A nested QMainWindow needs a central widget or it leaves an empty gap
    // the docks won't fill; a zero-size placeholder makes the docks take all
    // the room.
    auto *areaFiller = new QWidget(m_documentArea);
    areaFiller->setMaximumSize(0, 0);
    m_documentArea->setCentralWidget(areaFiller);
    ui->centralwidget->layout()->addWidget(m_documentArea);
    // Watch for the dock tab bars Qt creates on the fly (when docks get
    // grouped) so "drag a tab out to detach it" can be wired onto each
    // (see installDockTabBarFilters()).
    m_documentArea->installEventFilter(this);
    // The first document - createDocument() activates it, which is what
    // populates sheetScene/placementController & co. for everything below.
    createDocument();

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

    // Object snap (endpoints/midpoints/centers/intersections of what's
    // already drawn - see Sheet::snapPosition()); on by default, persisted
    // like the grid snap right above.
    const QVariant objectSnapEnabled = SettingsManager::getInstance().loadSetting("snap_objects");
    const QSignalBlocker blockObjectSnapAction(ui->actionSnapToObjects);
    ui->actionSnapToObjects->setChecked(objectSnapEnabled.isValid() ? objectSnapEnabled.toBool() : true);
    sheetScene->setObjectSnapEnabled(ui->actionSnapToObjects->isChecked());
    connect(ui->actionSnapToObjects, &QAction::toggled, this, [this](bool checked) {
        SettingsManager::getInstance().saveSetting("snap_objects", checked);
        sheetScene->setObjectSnapEnabled(checked);
    });

    // Options-dialog settings that map onto live widget state (toolbar icon
    // size, mirrored toggles, ...) - see applyLiveSettings().
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &MainWindow::applyLiveSettings);
    applyLiveSettings();

    // Per-toolbar command customization (View > Customize toolbars...).
    loadToolbarCustomizations();
    connect(ui->actionCustomizeToolbars, &QAction::triggered,
            this, &MainWindow::clickCustomizeToolbarsAction);

    // Per-command shortcut customization (Help > Keyboard shortcuts).
    loadShortcutCustomizations();

    // (Each document's undo limit is set at creation - see createDocument().)

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
    // Sheets are owned (and deleted) by their Documents, which are QObject
    // children of this window.
    delete layerToolBarWidget;
    delete ui;
}

int MainWindow::nextUntitledNumber() const
{
    QSet<int> used;
    for (Document *document : m_documents) {
        if (document->filePath().isEmpty() && document->untitledNumber() > 0)
            used.insert(document->untitledNumber());
    }
    int number = 1;
    while (used.contains(number))
        ++number;
    return number;
}

Document *MainWindow::createDocument()
{
    auto *document = new Document(this);
    document->setUntitledNumber(nextUntitledNumber());
    Sheet *sheet = document->sheet();
    sheet->setSceneRect(0, 0, 5000, 5000); // fix the scene dimensions
    // 0 = unlimited (QUndoStack's own default). Only effective while the
    // stack is empty, hence set at creation; a change from the Options
    // dialog takes effect for documents opened afterwards.
    sheet->undoStack()->setUndoLimit(
            qMax(0, SettingsManager::getInstance().loadSetting("undo_limit").toInt()));
    sheet->setObjectSnapEnabled(ui->actionSnapToObjects->isChecked());
    // Same live-settings values applyLiveSettings()/the ctor lambdas push
    // into the active sheet - a document opened later must not start stale.
    const qreal lineWidth = SettingsManager::getInstance().loadSetting("line_width").toDouble();
    sheet->setLineWidth(lineWidth > 0 ? lineWidth : 0.5);
    const qreal connectionDiameter =
            SettingsManager::getInstance().loadSetting("connection_diameter").toDouble();
    sheet->setConnectionDiameter(connectionDiameter > 0 ? connectionDiameter : 2.0);

    auto *viewWidget = new DocumentView(m_documentArea);
    document->viewWidget = viewWidget;
    SheetView *view = viewWidget->view();
    view->setScene(sheet);
    // The view accepts macro drags itself (SheetView's drag handlers) and
    // deliberately ignores everything else, so file drops still bubble up
    // to this window's own dragEnterEvent()/dropEvent().

    // Welcome card over the still-empty drawing ("welcome_enabled" setting,
    // on by default): recent files, quick tips. It dismisses itself on any
    // click; here it is also dismissed as soon as the drawing gains content
    // (drawing, pasting, a file loaded into this document) or a drawing
    // tool is picked (see the toolbar hook in the constructor).
    const QVariant welcomeVal = SettingsManager::getInstance().loadSetting("welcome_enabled");
    if (!welcomeVal.isValid() || welcomeVal.toBool()) {
        auto *welcome = new WelcomeWidget(viewWidget);
        document->welcome = welcome;
        connect(welcome, &WelcomeWidget::openFileRequested,
                this, [this](const QString &path) { openFile(path); });
        connect(sheet, &Sheet::primitivesChanged, welcome, [document]() {
            if (document->welcome && !document->sheet()->primitives().isEmpty())
                document->welcome->dismiss();
        });
    }

    document->placement = new PrimitivePlacementController(
                view, sheet, ui->toolBarPrimitive, ui->cbPropLayer, ui->checkBox, document);
    view->setPlacementController(document->placement);
    // Measure-tool readout goes to the status bar's message area.
    connect(document->placement, &PrimitivePlacementController::measureUpdated,
            this, [this](const QString &text) { ui->statusbar->showMessage(text); });
    document->handles = new SelectionHandleController(sheet, document);

    // Per-view wiring, alive for the document's whole lifetime: a hidden
    // tab's view receives no mouse events, so these never fire for
    // background documents.
    connect(view, &SheetView::mouseMoved, ui->statusbar, &StatusBar::sceneMousePos);
    connect(view, &SheetView::zoomScaleIsChanged, ui->statusbar, &StatusBar::zoomLevel);
    connect(view, &SheetView::viewTransformChanged, this, &MainWindow::updateRulers);
    connect(view, &SheetView::contextMenuRequested, this, &MainWindow::showCanvasContextMenu);
    // A macro dragged from the Libraries panel and dropped here becomes an
    // instance at the (already snapped) drop position, as one undo step.
    // Activating the document first keeps the undo group and the active-
    // document mirrors pointing at the drawing that actually received it.
    connect(view, &SheetView::macroDropped, this,
            [this, document](const QString &macroKey, const QPointF &scenePos) {
        setActiveDocument(document);
        auto *macro = new PrimitiveMacro();
        macro->setMacroName(macroKey);
        macro->setControlPoint(0, scenePos);
        document->sheet()->addPrimitive(macro);
        document->sheet()->undoStack()->push(
                new CreatePrimitiveCommand(document->sheet(), macro));
    });
    connect(view, &SheetView::mouseMoved, this, [viewWidget](QPointF scenePos) {
        viewWidget->horizontalRuler()->setMarkerPosition(scenePos.x(), true);
        viewWidget->verticalRuler()->setMarkerPosition(scenePos.y(), true);
    });
    connect(view, &SheetView::mouseLeftView, this, [viewWidget]() {
        viewWidget->horizontalRuler()->setMarkerPosition(0, false);
        viewWidget->verticalRuler()->setMarkerPosition(0, false);
    });

    // Guides: dragging out of a ruler creates a guide line that follows the
    // cursor; releasing it over the drawing keeps it, anywhere else (e.g.
    // back on the ruler) discards it. The guide's coordinate is grid-snapped
    // so dropped guides land on round positions.
    auto guideCoordinate = [view](const QPoint &globalPos, Qt::Orientation orientation) {
        const QPointF scenePos = view->mapToScene(
                view->viewport()->mapFromGlobal(globalPos));
        const QPointF snapped = view->snapToGrid(scenePos);
        return orientation == Qt::Vertical ? snapped.x() : snapped.y();
    };
    auto wireRulerGuides = [this, view, sheet, guideCoordinate](RulerWidget *ruler,
                                                                Qt::Orientation orientation) {
        connect(ruler, &RulerWidget::guideDragStarted, this, [this]() {
            m_rulerGuideIndex = -1;
        });
        connect(ruler, &RulerWidget::guideDragMoved, this,
                [this, sheet, orientation, guideCoordinate](const QPoint &globalPos) {
            const qreal position = guideCoordinate(globalPos, orientation);
            if (m_rulerGuideIndex < 0)
                m_rulerGuideIndex = sheet->addGuide(orientation, position);
            else
                sheet->moveGuide(m_rulerGuideIndex, position);
        });
        connect(ruler, &RulerWidget::guideDragFinished, this,
                [this, view, sheet](const QPoint &globalPos) {
            if (m_rulerGuideIndex >= 0
                    && !view->viewport()->rect().contains(
                            view->viewport()->mapFromGlobal(globalPos)))
                sheet->removeGuide(m_rulerGuideIndex);
            m_rulerGuideIndex = -1;
        });
    };
    wireRulerGuides(viewWidget->horizontalRuler(), Qt::Horizontal);
    wireRulerGuides(viewWidget->verticalRuler(), Qt::Vertical);

    m_undoGroup->addStack(sheet->undoStack());
    // The dock title's dirty star follows this document's own stack,
    // active or not.
    connect(sheet->undoStack(), &QUndoStack::cleanChanged,
            this, [this, document]() { updateDocumentTitle(document); });

    // The dock hosting this document's view. Closable/movable/floatable so
    // it can be split out or floated with native dock dragging; DockWidget
    // has no title-bar-less mode that still allows dragging, so the small
    // title bar stays (it also carries the close button).
    auto *dock = new QDockWidget(document->displayName(), m_documentArea);
    dock->setWidget(viewWidget);
    dock->setFeatures(QDockWidget::DockWidgetMovable
                      | QDockWidget::DockWidgetFloatable
                      | QDockWidget::DockWidgetClosable);
    // Intercept the dock's own close (its title-bar X) to run the
    // unsaved-changes prompt and actually remove the document.
    dock->installEventFilter(this);
    // Activate this document when its view is clicked or focused - the
    // active document is "the one being worked on", which drives every
    // panel and the ~90 action handlers.
    view->viewport()->installEventFilter(this);
    view->installEventFilter(this);
    connect(dock, &QDockWidget::visibilityChanged, this, [this, document](bool visible) {
        if (visible)
            setActiveDocument(document);
    });
    document->dock = dock;

    m_documentArea->addDockWidget(Qt::TopDockWidgetArea, dock);
    // Default grouping: every new document is stacked as a TAB on top of
    // the most recently opened one (not placed side by side) - the user
    // splits them apart by dragging a dock out. m_documents still holds
    // only the earlier documents here (the new one is appended just below),
    // so last() is the correct dock to tab onto.
    if (!m_documents.isEmpty()) {
        if (Document *previous = m_documents.last(); previous->dock)
            m_documentArea->tabifyDockWidget(previous->dock, dock);
    }

    // Re-evaluate title bars/tab close buttons after the layout change, and
    // keep doing so whenever this dock is (un)grouped, floated or moved.
    connect(dock, &QDockWidget::dockLocationChanged, this, [this]() {
        setupDockTabBars();
        updateDockTitleBars();
    });
    connect(dock, &QDockWidget::topLevelChanged, this, [this]() {
        setupDockTabBars();
        updateDockTitleBars();
    });

    m_documents.append(document);
    updateRulersVisibility(); // apply the rulers setting to the new view too
    dock->raise();            // bring the new document's tab to the front
    setActiveDocument(document);
    setupDockTabBars();       // tabifying above may have created a tab bar
    // Deferred: tabifiedDockWidgets() only reflects the tabify above after
    // Qt has re-laid-out the docks, so a synchronous updateDockTitleBars()
    // here would still see the just-grouped docks as ungrouped (leaving
    // their title bars up until the next event).
    QTimer::singleShot(0, this, [this]() { updateDockTitleBars(); });
    return document;
}

QList<QDockWidget *> MainWindow::allManagedDocks() const
{
    QList<QDockWidget *> docks;
    for (Document *document : m_documents) {
        if (document->dock)
            docks.append(document->dock);
    }
    docks << ui->dockLibraries << ui->dockProperties << ui->dockFcdCode << ui->dockLayers;
    return docks;
}

QMainWindow *MainWindow::areaForDock(QDockWidget *dock) const
{
    // Document docks live in the nested area; the side panels on the window.
    for (Document *document : m_documents) {
        if (document->dock == dock)
            return m_documentArea;
    }
    return const_cast<MainWindow *>(this);
}

QDockWidget *MainWindow::dockForTabText(const QString &text) const
{
    // A dock tab's text is exactly its dock's window title.
    for (QDockWidget *dock : allManagedDocks()) {
        if (dock->windowTitle() == text)
            return dock;
    }
    return nullptr;
}

void MainWindow::setupDockTabBars()
{
    // Every dock tab bar (Qt creates/destroys them as docks are grouped),
    // in both the document area and the window itself, gets close buttons
    // and the tear-off event filter - once each.
    QList<QTabBar *> bars = m_documentArea->findChildren<QTabBar *>();
    bars += findChildren<QTabBar *>();
    for (QTabBar *bar : bars) {
        if (bar->property("eschemaDockTabs").toBool())
            continue;
        // A dock tab bar's tabs carry dock window titles; skip any other
        // QTabBar (there are none today, but stay safe).
        bar->setProperty("eschemaDockTabs", true);
        bar->setTabsClosable(true);
        // A faint grey X always visible; hovering it (only the X, not the
        // whole tab) turns it white on a red pill. The image must be given
        // explicitly - styling the close-button's background drops Qt's own
        // X glyph.
        bar->setStyleSheet(QStringLiteral(
            "QTabBar::close-button {"
            " image: url(:/res/resources/tab_close.png);"
            " background: transparent; border-radius: 7px; }"
            "QTabBar::close-button:hover {"
            " image: url(:/res/resources/tab_close_hover.png);"
            " background: #e04043; }"));
        connect(bar, &QTabBar::tabCloseRequested, this, [this, bar](int index) {
            QDockWidget *dock = dockForTabText(bar->tabText(index));
            if (!dock)
                return;
            // A document dock runs the unsaved-changes flow; a side panel
            // just hides (reopenable from the View menu).
            if (Document *document = documentForObject(dock))
                closeDocument(document);
            else
                dock->close();
        });
        // Right-clicking a tab: document tabs get their own close/copy-path
        // menu; panel tabs keep the standard panels/toolbars menu.
        bar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(bar, &QTabBar::customContextMenuRequested, this, [this, bar](const QPoint &pos) {
            showDocumentTabContextMenu(bar, pos);
        });
    }
}

void MainWindow::showDocumentTabContextMenu(QTabBar *bar, const QPoint &pos)
{
    const int index = bar->tabAt(pos);
    if (index < 0)
        return;
    QDockWidget *dock = dockForTabText(bar->tabText(index));
    if (!dock)
        return;
    Document *document = documentForObject(dock);
    if (!document) {
        // A side panel's tab: show the same panels/toolbars visibility menu
        // a right-click on the toolbar area shows, rather than nothing (the
        // custom context-menu policy set above suppresses Qt's default).
        if (QMenu *panelMenu = createPopupMenu()) {
            panelMenu->setAttribute(Qt::WA_DeleteOnClose);
            panelMenu->popup(bar->mapToGlobal(pos));
        }
        return;
    }

    QMenu menu(this);
    QAction *closeAction = menu.addAction(tr("Close"));
    QAction *closeOthersAction = menu.addAction(tr("Close the others"));
    QAction *closeAllAction = menu.addAction(tr("Close all"));
    closeOthersAction->setEnabled(m_documents.size() > 1);
    menu.addSeparator();
    // Only meaningful for a document that actually lives on disk.
    const bool hasPath = !document->filePath().isEmpty();
    QAction *copyPathAction = menu.addAction(tr("Copy file path"));
    QAction *openFolderAction = menu.addAction(tr("Open containing folder"));
    copyPathAction->setEnabled(hasPath);
    openFolderAction->setEnabled(hasPath);

    QAction *chosen = menu.exec(bar->mapToGlobal(pos));
    if (!chosen)
        return;
    if (chosen == closeAction) {
        closeDocument(document);
    } else if (chosen == closeOthersAction || chosen == closeAllAction) {
        // Iterate a snapshot - closeDocument() mutates m_documents (and
        // "close all" of the last one resets it to a fresh untitled instead,
        // per the one-document-always-open rule). Each dirty document still
        // asks about its unsaved changes; cancelling skips just that one.
        const QList<Document *> snapshot = m_documents;
        for (Document *other : snapshot) {
            if (chosen == closeOthersAction && other == document)
                continue;
            if (m_documents.contains(other))
                closeDocument(other);
        }
    } else if (chosen == copyPathAction) {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(document->filePath()));
    } else if (chosen == openFolderAction) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(document->filePath()).absolutePath()));
    }
}

void MainWindow::applyDockTitleBar(QDockWidget *dock, QMainWindow *area)
{
    // The fat native title bar is never shown. What a dock gets instead:
    // - tabbed (and not floating): nothing at all - its tab is the handle
    //   and carries the close button;
    // - the ONLY open document (and not floating): nothing - there is no
    //   layout to reorganize and it must not be closable away;
    // - otherwise (floating, or a lone split pane next to other docks): a
    //   slim tab-looking DockTitleTab as drag handle + close button.
    const bool floating = dock->isFloating();
    const bool tabbed = !area->tabifiedDockWidgets(dock).isEmpty();
    const bool onlyDocument = (area == m_documentArea) && m_documents.size() <= 1;
    const bool wantNone = !floating && (tabbed || onlyDocument);

    QWidget *current = dock->titleBarWidget();
    const bool currentIsTab = qobject_cast<DockTitleTab *>(current) != nullptr;

    if (wantNone) {
        if (!current || currentIsTab) {
            auto *empty = new QWidget(dock);
            empty->setFixedHeight(0);
            dock->setTitleBarWidget(empty);
            if (current)
                current->deleteLater();
        }
    } else if (!currentIsTab) {
        auto *tab = new DockTitleTab(dock);
        connect(tab, &DockTitleTab::closeRequested, this, [this, dock]() {
            // A document runs the unsaved-changes flow; a side panel just
            // hides (reopenable from the View menu).
            if (Document *document = documentForObject(dock))
                closeDocument(document);
            else
                dock->close();
        });
        dock->setTitleBarWidget(tab);
        if (current)
            current->deleteLater();
    }
}

void MainWindow::updateDockTitleBars()
{
    for (QDockWidget *dock : allManagedDocks())
        applyDockTitleBar(dock, areaForDock(dock));
}

Document *MainWindow::documentForObject(QObject *object) const
{
    for (Document *document : m_documents) {
        if (object == document->dock)
            return document;
        if (document->viewWidget) {
            SheetView *view = document->viewWidget->view();
            if (object == view || object == view->viewport())
                return document;
        }
    }
    return nullptr;
}

SheetView *MainWindow::activeView() const
{
    return m_activeDocument->viewWidget->view();
}

DocumentView *MainWindow::activeDocumentView() const
{
    return m_activeDocument->viewWidget;
}

void MainWindow::setActiveDocument(Document *document)
{
    if (!document || document == m_activeDocument || !m_documents.contains(document))
        return;

    if (m_activeDocument) {
        // Whatever placement/preview the outgoing document had in flight
        // must not stay armed while it's no longer the active one.
        if (m_activeDocument->placement) {
            m_activeDocument->placement->cancelPlacement();
            m_activeDocument->placement->setActive(false);
        }
        // Freeze its layer definitions so the incoming document's don't
        // clobber them (see Document's class comment).
        m_activeDocument->captureLayerState();
    }
    for (const QMetaObject::Connection &connection : std::as_const(m_documentConnections))
        disconnect(connection);
    m_documentConnections.clear();

    m_activeDocument = document;
    sheetScene = document->sheet();
    placementController = document->placement;
    selectionHandleController = document->handles;
    currentFilePath = document->filePath();
    placementController->setActive(true);
    document->applyLayerState();
    m_undoGroup->setActiveStack(sheetScene->undoStack());

    // Connections that must only ever fire for the ACTIVE document - a
    // background load/edit must not hijack the panels or the status bar.
    m_documentConnections << connect(sheetScene, &Sheet::primitivesChanged, this, [this]() {
        int macroCount = 0;
        for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib)
                ++macroCount;
        }
        ui->statusbar->primitiveCounts(sheetScene->primitives().size(), macroCount);
    });
    // Coalesced, not direct: see scheduleSelectionRefresh()'s own comment.
    m_documentConnections << connect(sheetScene, &QGraphicsScene::selectionChanged,
                                     this, &MainWindow::scheduleSelectionRefresh);
    // Alt+drag duplicate: the drag itself is handled inside
    // GraphicsPrimitive; dropping the in-place copy needs the FCD
    // serialize/parse round trip, which lives here.
    m_documentConnections << connect(sheetScene, &Sheet::altDragCloneRequested,
                                     this, &MainWindow::cloneSelectionInPlace);
    m_documentConnections << connect(sheetScene->undoStack(), &QUndoStack::indexChanged,
                                     this, &MainWindow::refreshFcdCodeIfClean);
    // A drag-resize/move only lands on the undo stack at mouse release, and
    // never fires selectionChanged - without this the panel's new geometry
    // fields (length/width/height) would keep showing pre-drag values.
    m_documentConnections << connect(sheetScene->undoStack(), &QUndoStack::indexChanged,
                                     this, &MainWindow::updatePropertiesPanel);

    // Refresh every per-document surface for the new active document.
    updateWindowTitle();
    updateDocumentTitle(document);
    updateRulers();
    syncFcdCodeFromSheet();
    scheduleSelectionRefresh();
    int macroCount = 0;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib)
            ++macroCount;
    }
    ui->statusbar->primitiveCounts(sheetScene->primitives().size(), macroCount);
}

void MainWindow::updateDocumentTitle(Document *document)
{
    if (!document || !document->dock)
        return;
    QString title = document->displayName();
    if (!document->sheet()->undoStack()->isClean())
        title += QLatin1Char('*');
    document->dock->setWindowTitle(title);
    document->dock->setToolTip(document->filePath());
    if (document == m_activeDocument)
        updateWindowTitle();
}

void MainWindow::resetActiveDocument()
{
    if (placementController)
        placementController->cancelPlacement();
    sheetScene->clearPrimitives();
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(QString());
    clearAutosaveFor(m_activeDocument);
    // setClean() alone doesn't fire indexChanged() - the FCD code dock would
    // otherwise keep showing the previous document.
    syncFcdCodeFromSheet();
    updateDocumentTitle(m_activeDocument);
}

void MainWindow::closeDocument(Document *document)
{
    if (!document)
        return;

    // confirmDiscardChanges() works on the active document - make the one
    // being closed active first, which also gives the user visual context
    // for the save/discard question.
    setActiveDocument(document);
    if (!confirmDiscardChanges())
        return;

    // The last remaining document never closes - it resets to a fresh
    // untitled drawing instead, so there is always something to draw on.
    if (m_documents.size() == 1) {
        resetActiveDocument();
        return;
    }

    // Pick the document to activate in its place (the next one, or the
    // previous when closing the last) before tearing this one down.
    const int index = m_documents.indexOf(document);
    Document *next = m_documents.value(index + 1 < m_documents.size() ? index + 1 : index - 1);

    // A confirmed close (saved or deliberately discarded) - its recovery
    // sidecar is moot either way.
    clearAutosaveFor(document);
    m_documents.removeAll(document);
    m_undoGroup->removeStack(document->sheet()->undoStack());
    // m_activeDocument is about to dangle - clear it so setActiveDocument()
    // below doesn't try to deactivate a half-deleted document.
    if (m_activeDocument == document)
        m_activeDocument = nullptr;
    if (document->dock) {
        m_documentArea->removeDockWidget(document->dock);
        document->dock->deleteLater(); // also deletes its DocumentView child
    }
    document->deleteLater();

    setActiveDocument(next);
    // The survivors' chrome may change class (e.g. a pane whose tab-mate
    // just left becomes a lone pane; the very last document loses its tab):
    // re-evaluate once the layout has settled.
    setupDockTabBars();
    QTimer::singleShot(0, this, [this]() { updateDockTitleBars(); });
}

void MainWindow::setConnections()
{
    connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::clickOptionAction);
    connect(ui->actionInformation, &QAction::triggered, this, &MainWindow::clickAboutAction);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionCheckUpdates, &QAction::triggered, this, &MainWindow::clickCheckUpdatesAction);
    connect(ui->actionCommandPalette, &QAction::triggered, this, &MainWindow::clickCommandPaletteAction);
    // The command palette's launcher in the menu bar's unused space: the
    // palette opens the moment the user starts TYPING in the field (never
    // on mere focus - Qt shifts focus here in many programmatic cases, e.g.
    // switching document tabs, and popping the palette on those was wrong),
    // carrying the first character over. Positioned manually just after the
    // last menu (QMenuBar's corner widget would pin it to the far right
    // instead) - see positionMenuBarSearch(), re-run on every menu bar
    // resize via the event filter.
    m_menuBarSearch = new QLineEdit(ui->menubar);
    m_menuBarSearch->setObjectName(QStringLiteral("menuBarSearch"));
    m_menuBarSearch->setPlaceholderText(tr("Search commands..."));
    m_menuBarSearch->setToolTip(ui->actionCommandPalette->toolTip());
    m_menuBarSearch->setFixedWidth(220);
    ui->menubar->installEventFilter(this);
    positionMenuBarSearch();
    // textEdited fires only for real user input, never for the clear()
    // below or any other programmatic setText() - so this can't loop.
    connect(m_menuBarSearch, &QLineEdit::textEdited, this, [this](const QString &typed) {
        if (m_menuSearchOpening || typed.isEmpty())
            return;
        m_menuSearchOpening = true;
        m_menuBarSearch->clear();
        // Move focus off the field first, so closing the palette can't
        // land focus back on it.
        activeView()->setFocus();
        const QPoint anchor = m_menuBarSearch->mapToGlobal(
                QPoint(m_menuBarSearch->width() / 2, m_menuBarSearch->height() + 2));
        openCommandPalette(anchor, typed);
        m_menuSearchOpening = false;
    });
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &MainWindow::updateRulersVisibility);
    connect(ui->actionClearGuides, &QAction::triggered, this, [this]() {
        sheetScene->clearGuides();
    });
    // Persists across restarts (see the constructor's own read of the same
    // key) - toggled() only fires on an actual state change, so the initial
    // setChecked() there can't be relied on to also save it back.
    connect(ui->actionRulersVisible, &QAction::toggled, this, [](bool checked) {
        SettingsManager::getInstance().saveSetting("rulers_visible", checked);
    });
    connect(ui->actionAdjustView, &QAction::triggered,
            this, [this]() { activeView()->adjustView(); });
    connect(ui->actionZoomToSelection, &QAction::triggered,
            this, [this]() { activeView()->adjustViewToSelection(); });
    // Clicking the status bar's zoom readout opens a quick zoom menu:
    // preset levels, the two fit actions (reused, so icons/shortcuts/
    // enabled state match the View menu), and an exact custom value.
    connect(ui->statusbar, &StatusBar::zoomWidgetClicked,
            this, [this](const QPoint &globalPos) {
        QMenu menu(this);
        for (int percent : { 10, 25, 50, 75, 100 }) {
            menu.addAction(tr("Zoom %1%").arg(percent), this,
                           [this, percent]() { activeView()->setZoomPercent(percent); });
        }
        menu.addSeparator();
        menu.addAction(ui->actionAdjustView);
        menu.addAction(ui->actionZoomToSelection);
        menu.addSeparator();
        menu.addAction(tr("Custom zoom..."), this, [this]() {
            bool ok = false;
            const int percent = QInputDialog::getInt(this, tr("Custom zoom"),
                    tr("Zoom level (%):"), activeView()->zoomPercent(), 2, 100, 1, &ok);
            if (ok)
                activeView()->setZoomPercent(percent);
        });
        menu.exec(globalPos);
    });
    connect(ui->actionLayerManager, &QAction::triggered, this, &MainWindow::clickLayerManagerAction);
    connect(ui->actionAttachImage, &QAction::triggered, this, &MainWindow::clickAttachImageAction);
    connect(ui->actionSymbolWizard, &QAction::triggered, this, &MainWindow::clickSymbolWizardAction);
    connect(ui->actionShortcuts, &QAction::triggered, this, &MainWindow::clickShortcutsAction);
    connect(ui->btnApplyFcdCode, &QPushButton::clicked, this, &MainWindow::clickApplyFcdCodeAction);
    connect(ui->btnRefreshFcdCode, &QPushButton::clicked, this, &MainWindow::clickRefreshFcdCodeAction);
    connect(ui->actionMirror, &QAction::triggered, this, &MainWindow::clickMirrorAction);
    connect(ui->actionMirrorCopy, &QAction::triggered, this, &MainWindow::clickMirrorCopyAction);
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
    connect(ui->actionHatch, &QAction::triggered, this, &MainWindow::clickHatchAction);
    connect(ui->actionRotateByAngle, &QAction::triggered, this, &MainWindow::clickRotateByAngleAction);
    connect(ui->actionScaleSelection, &QAction::triggered, this, &MainWindow::clickScaleSelectionAction);
    connect(ui->actionMoveBasePoint, &QAction::triggered, this, &MainWindow::clickMoveBasePointAction);
    connect(ui->actionCopyBasePoint, &QAction::triggered, this, &MainWindow::clickCopyBasePointAction);
    connect(ui->actionArray, &QAction::triggered, this, &MainWindow::clickArrayAction);
    connect(ui->actionSnapSelectionToGrid, &QAction::triggered, this, &MainWindow::clickSnapSelectionToGridAction);
    connect(ui->actionConvertToPolygon, &QAction::triggered, this, &MainWindow::clickConvertToPolygonAction);
    connect(ui->actionConvertToCurve, &QAction::triggered, this, &MainWindow::clickConvertToCurveAction);
    connect(ui->actionSimplifyNodes, &QAction::triggered, this, &MainWindow::clickSimplifyNodesAction);
    connect(ui->actionFilletCorners, &QAction::triggered, this, &MainWindow::clickFilletCornersAction);
    connect(ui->actionChamferCorners, &QAction::triggered, this, &MainWindow::clickChamferCornersAction);
    connect(ui->actionOffsetOutline, &QAction::triggered, this, &MainWindow::clickOffsetOutlineAction);
    connect(ui->actionSplitAtPoint, &QAction::triggered, this, &MainWindow::clickSplitAtPointAction);
    connect(ui->actionTrimToIntersection, &QAction::triggered, this, &MainWindow::clickTrimToIntersectionAction);
    connect(ui->actionExtendToIntersection, &QAction::triggered, this, &MainWindow::clickExtendToIntersectionAction);
    connect(ui->actionSelectSameType, &QAction::triggered, this, &MainWindow::clickSelectSameTypeAction);
    connect(ui->actionInvertSelection, &QAction::triggered, this, &MainWindow::clickInvertSelectionAction);
    connect(ui->actionConvertMacroToPrimitives, &QAction::triggered, this, &MainWindow::clickConvertMacroToPrimitivesAction);
    connect(ui->actionCreateMacro, &QAction::triggered, this, &MainWindow::clickCreateMacroAction);
    // Keeps every selection/clipboard-dependent Edit action's enabled state
    // current - both for the menu bar and the right-click context menu,
    // which reuses these same QAction objects (see showCanvasContextMenu()).
    //
    // Coalesced, not direct: a mass selection change (Ctrl+A, invert,
    // delete-many) emits selectionChanged once *per primitive*, and running
    // the two O(n) refreshers below on each emission made those operations
    // O(n²) - a multi-second freeze on large drawings. One queued refresh
    // per event-loop turn covers the whole batch (same pattern as
    // SettingsManager's coalesced change signal).
    // (The selectionChanged connection itself is per-document - see
    // setActiveDocument().)
    // The clipboard state is cached here (not read inside
    // updateEditActionsState()): querying the system clipboard is an OS
    // roundtrip, far too expensive for something that used to run once per
    // primitive during mass selections.
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, [this]() {
        const QClipboard *clipboard = QGuiApplication::clipboard();
        m_clipboardPastable = !clipboard->text().isEmpty()
                || clipboard->mimeData()->hasImage();
        updateEditActionsState();
    });
    {
        const QClipboard *clipboard = QGuiApplication::clipboard();
        m_clipboardPastable = !clipboard->text().isEmpty()
                || clipboard->mimeData()->hasImage();
    }
    updateEditActionsState();

    // (contextMenuRequested is wired per view in createDocument().)

    // Properties panel: reflects the selection, and each field applies its
    // edit back to every selected primitive when the user actually changes
    // it (editingFinished()/activated()-style signals, not textChanged() -
    // so partially-typed text or a value the sync itself just set doesn't
    // get pushed back out on every keystroke). Refreshed through the same
    // coalesced path as the edit actions above.
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
    connect(ui->comboArrowStyle, &ArrowStyleComboBox::arrowStyleChanged, this, [this](int style) {
        // FCJ style bits: 0x01 limiter bar, 0x02 empty triangle.
        for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
            if (primitive->supportsArrows()) {
                primitive->setArrowStyleLimiter(style & 0x01);
                primitive->setArrowStyleEmpty(style & 0x02);
            }
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
    connect(ui->actionPasteInPlace, &QAction::triggered, this, &MainWindow::clickPasteInPlaceAction);
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
    // Hovering a recent-file entry shows its thumbnail beside the menu.
    // Wired once here (the menu object survives rebuilds; its actions don't).
    connect(ui->menuRecentFiles, &QMenu::hovered, this, &MainWindow::showRecentFilePreview);
    connect(ui->menuRecentFiles, &QMenu::aboutToHide, this, [this]() {
        if (m_recentPreview)
            m_recentPreview->hide();
    });
    connect(ui->actionImportDxf, &QAction::triggered, this, &MainWindow::clickImportDxfAction);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::clickSaveAction);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::clickSaveAsAction);
    connect(ui->actionSaveSplit, &QAction::triggered, this, &MainWindow::clickSaveSplitAction);
    connect(ui->actionPrint, &QAction::triggered, this, &MainWindow::clickPrintAction);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::clickExportAction);
    // Ctrl+W closes the active document (the standard gesture) - with only
    // one left it resets to a fresh untitled drawing.
    connect(ui->actionClose, &QAction::triggered, this, [this]() {
        closeDocument(m_activeDocument);
    });
    // QWidget::close() reaches closeEvent(), which runs the per-document
    // unsaved-changes checks - so File > Exit and the window's own titlebar
    // X button behave identically.
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionSaveAll, &QAction::triggered, this, &MainWindow::clickSaveAllAction);
    // Window menu: cycling shortcuts plus a per-document list rebuilt on
    // every open (see rebuildWindowMenu()).
    connect(ui->actionNextDocument, &QAction::triggered,
            this, [this]() { cycleActiveDocument(1); });
    connect(ui->actionPreviousDocument, &QAction::triggered,
            this, [this]() { cycleActiveDocument(-1); });
    connect(ui->menuWindow, &QMenu::aboutToShow, this, &MainWindow::rebuildWindowMenu);

    // Undo/Redo through the QUndoGroup: whichever tab is active supplies
    // the group's active stack (see setActiveDocument()), so the two
    // actions, their enabled state and their command-name texts always
    // reflect the current document.
    connect(ui->actionUndo, &QAction::triggered, m_undoGroup, &QUndoGroup::undo);
    connect(ui->actionRestore, &QAction::triggered, m_undoGroup, &QUndoGroup::redo);
    connect(m_undoGroup, &QUndoGroup::canUndoChanged, ui->actionUndo, &QAction::setEnabled);
    connect(m_undoGroup, &QUndoGroup::canRedoChanged, ui->actionRestore, &QAction::setEnabled);
    connect(m_undoGroup, &QUndoGroup::undoTextChanged, this, [this](const QString &text) {
        ui->actionUndo->setText(text.isEmpty() ? tr("Undo") : tr("Undo: %1").arg(text));
    });
    connect(m_undoGroup, &QUndoGroup::redoTextChanged, this, [this](const QString &text) {
        ui->actionRestore->setText(text.isEmpty() ? tr("Redo") : tr("Redo: %1").arg(text));
    });
    ui->actionUndo->setEnabled(m_undoGroup->canUndo());
    ui->actionRestore->setEnabled(m_undoGroup->canRedo());

    // refreshFcdCodeIfClean() skips its (whole-document) serialization
    // while the dock is hidden - catch up the moment it's shown again.
    // (The per-stack indexChanged connections live in setActiveDocument().)
    connect(ui->dockFcdCode, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (visible)
            refreshFcdCodeIfClean();
    });
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
    QList<DialogShortcuts::Command> commands;
    const auto catalog = commandCatalogByCategory();
    for (const auto &entry : catalog) {
        commands.append({ entry.first, entry.second,
                          m_defaultShortcuts.value(entry.second) });
    }
    shortcutsDialog = new DialogShortcuts(commands, this);
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
    // The layer manager is a dockable panel (ui->dockLayers, hidden by
    // default) rather than a separate dialog - this action shows it and
    // brings it to the front of whatever tab group it lives in, so layers
    // can be managed side by side with the drawing.
    ui->dockLayers->setVisible(true);
    ui->dockLayers->raise();
    if (ui->dockLayers->isFloating())
        ui->dockLayers->activateWindow();
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

void MainWindow::clickSymbolWizardAction()
{
    DialogSymbolWizard dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QList<GraphicsPrimitive *> primitives = dialog.buildPrimitives();
    if (primitives.isEmpty())
        return;

    // Anchor at the library format's conventional (100,100) macro origin -
    // same convention as Create macro from selection.
    QRectF bounds;
    bool first = true;
    for (GraphicsPrimitive *primitive : std::as_const(primitives)) {
        for (int i = 0; i < primitive->controlPointCount(); ++i) {
            const QPointF point = primitive->controlPoint(i);
            if (first) {
                bounds = QRectF(point, QSizeF(0, 0));
                first = false;
            } else {
                bounds = bounds.united(QRectF(point, QSizeF(0, 0)));
            }
        }
    }
    const QPointF offset = QPointF(100, 100) - bounds.topLeft();
    for (GraphicsPrimitive *primitive : std::as_const(primitives))
        primitive->translateControlPoints(offset);
    const QString body = FidoCadWriter::writeSelection(primitives);
    qDeleteAll(primitives);

    // Random key, retried against a collision - same scheme as the
    // reference FidoCadJ editor's own LibraryModel.createRandomMacroKey().
    const QString libraryFilename = dialog.libraryFilename();
    QString key;
    for (int attempt = 0; attempt < 20; ++attempt) {
        key = QString::number(QRandomGenerator::global()->bounded(100000000));
        const QString fullKey = (libraryFilename + QLatin1Char('.') + key).toLower();
        if (!LibraryManager::getInstance().macro(fullKey))
            break;
    }

    QString errorMessage;
    const bool ok = LibraryManager::getInstance().addUserMacro(
                libraryFilename, dialog.libraryDisplayName(),
                key, dialog.macroName(), dialog.category(), body, &errorMessage);
    if (!ok)
        QMessageBox::warning(this, tr("Symbol wizard"), errorMessage);
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
    if (m_activeDocument) {
        m_activeDocument->setFilePath(filePath);
        updateDocumentTitle(m_activeDocument);
    }
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
    const int maxSetting = SettingsManager::getInstance().loadSetting("recent_files_max").toInt();
    const int maxRecents = maxSetting > 0 ? qBound(1, maxSetting, 30) : 10;
    while (recents.size() > maxRecents)
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
        // The path doubles as the hover-preview key - see
        // showRecentFilePreview(), wired once in setConnections().
        action->setData(path);
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
            openFile(path); // opens in its own tab, nothing to confirm
        }, Qt::QueuedConnection);
    }

    ui->menuRecentFiles->addSeparator();
    QAction *clearAction = ui->menuRecentFiles->addAction(tr("Clear list"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        SettingsManager::getInstance().saveSetting("recent_files", QStringList());
        updateRecentFilesMenu();
    }, Qt::QueuedConnection);
}

void MainWindow::showRecentFilePreview(QAction *action)
{
    // Only the file entries carry a path in their data; the separator and
    // "Clear list" hide any preview left up.
    const QString path = action ? action->data().toString() : QString();
    const QPixmap thumb = path.isEmpty() ? QPixmap()
            : DrawingThumbnails::thumbnail(path, QSize(200, 150));
    if (thumb.isNull()) {
        if (m_recentPreview)
            m_recentPreview->hide();
        return;
    }
    if (!m_recentPreview) {
        // A tooltip-flagged label: floats above the menu without stealing
        // activation from it.
        m_recentPreview = new QLabel(this, Qt::ToolTip | Qt::FramelessWindowHint);
        m_recentPreview->setFrameShape(QFrame::Box);
        m_recentPreview->setAttribute(Qt::WA_ShowWithoutActivating);
    }
    m_recentPreview->setPixmap(thumb);
    m_recentPreview->adjustSize();
    // Beside the hovered row, to the menu's right - flipped to its left
    // when that would run off the screen.
    const QRect actionRect = ui->menuRecentFiles->actionGeometry(action);
    QPoint position = ui->menuRecentFiles->mapToGlobal(actionRect.topRight()) + QPoint(10, 0);
    if (QScreen *scr = screen()) {
        const QRect available = scr->availableGeometry();
        if (position.x() + m_recentPreview->width() > available.right())
            position.setX(ui->menuRecentFiles->mapToGlobal(actionRect.topLeft()).x()
                          - m_recentPreview->width() - 10);
        position.setY(qMin(position.y(), available.bottom() - m_recentPreview->height()));
    }
    m_recentPreview->move(position);
    m_recentPreview->show();
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
    if (!m_activeDocument)
        return; // mid-construction: the first document isn't active yet
    SheetView *view = activeView();
    RulerWidget *rulerHorizontal = activeDocumentView()->horizontalRuler();
    RulerWidget *rulerVertical = activeDocumentView()->verticalRuler();
    const QPoint viewportOriginGlobal = view->viewport()->mapToGlobal(
                view->mapFromScene(QPointF(0, 0)));
    const QPoint originInHorizontalRuler = rulerHorizontal->mapFromGlobal(viewportOriginGlobal);
    const QPoint originInVerticalRuler = rulerVertical->mapFromGlobal(viewportOriginGlobal);

    const QTransform transform = view->transform();
    rulerHorizontal->setViewTransform(originInHorizontalRuler.x(), transform.m11());
    rulerVertical->setViewTransform(originInVerticalRuler.y(), transform.m22());

    const qreal minorStep = view->minorGridStep();
    const qreal majorStep = view->majorGridStep();
    rulerHorizontal->setGridSteps(minorStep, majorStep);
    rulerVertical->setGridSteps(minorStep, majorStep);
}

void MainWindow::updateRulersVisibility()
{
    const QVariant val = SettingsManager::getInstance().loadSetting("rulers_visible");
    const bool visible = val.isValid() ? val.toBool() : true;
    // Every document's rulers, not just the active tab's: the setting is
    // app-wide.
    for (Document *document : std::as_const(m_documents)) {
        if (!document->viewWidget)
            continue;
        document->viewWidget->horizontalRuler()->setVisible(visible);
        document->viewWidget->verticalRuler()->setVisible(visible);
        document->viewWidget->rulerCorner()->setVisible(visible);
    }
}

bool MainWindow::saveDocumentToPath(Document *document, const QString &filePath)
{
    // Optional safety net (Options > General): keep the previous on-disk
    // version as <name>.fcd.bak before overwriting it.
    if (SettingsManager::getInstance().loadSetting("save_backup").toBool()
            && QFile::exists(filePath)) {
        const QString backupPath = filePath + QStringLiteral(".bak");
        QFile::remove(backupPath);
        QFile::copy(filePath, backupPath);
    }

    QString error;
    if (!FidoCadWriter::writeFile(document->sheet(), filePath, &error)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to save the file:\n%1").arg(error));
        return false;
    }
    // Marks the undo stack's current position as "the saved state" - isClean()
    // (used by confirmDiscardChanges()) reports true again until the next
    // change, rather than staying permanently "dirty" after the first edit.
    document->sheet()->undoStack()->setClean();
    // The drawing is now safely on disk at filePath - any outstanding
    // recovery sidecar for it is redundant.
    clearAutosaveFor(document);
    updateDocumentTitle(document);
    return true;
}

bool MainWindow::saveToPath(const QString &filePath)
{
    return saveDocumentToPath(m_activeDocument, filePath);
}

void MainWindow::cycleActiveDocument(int step)
{
    const int count = m_documents.size();
    if (count < 2)
        return;
    int index = m_documents.indexOf(m_activeDocument);
    index = ((index < 0 ? 0 : index) + step + count) % count;
    Document *document = m_documents.at(index);
    if (document->dock)
        document->dock->raise();
    setActiveDocument(document);
}

void MainWindow::rebuildWindowMenu()
{
    // Drop the previous per-document entries (marked with a property so
    // the two static cycling actions and the separator survive).
    const QList<QAction *> actions = ui->menuWindow->actions();
    for (QAction *action : actions) {
        if (action->property("eschemaDocumentEntry").toBool()) {
            ui->menuWindow->removeAction(action);
            delete action;
        }
    }
    for (Document *document : std::as_const(m_documents)) {
        QString title = document->displayName();
        if (!document->sheet()->undoStack()->isClean())
            title += QLatin1Char('*');
        QAction *action = ui->menuWindow->addAction(title);
        action->setProperty("eschemaDocumentEntry", true);
        action->setCheckable(true);
        action->setChecked(document == m_activeDocument);
        connect(action, &QAction::triggered, this, [this, document]() {
            // Guard against a document closed while the menu was open.
            if (!m_documents.contains(document))
                return;
            if (document->dock)
                document->dock->raise();
            setActiveDocument(document);
        });
    }
}

void MainWindow::clickSaveAllAction()
{
    // Named dirty documents save silently in place; untitled ones need a
    // path, so each is activated (for visual context) and runs the regular
    // Save As flow - cancelling one just moves on to the next.
    const QList<Document *> documents = m_documents;
    for (Document *document : documents) {
        if (document->sheet()->undoStack()->isClean())
            continue;
        if (!document->filePath().isEmpty()) {
            saveDocumentToPath(document, document->filePath());
        } else {
            setActiveDocument(document);
            if (document->dock)
                document->dock->raise();
            clickSaveAsAction();
        }
    }
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

// QMainWindow builds a private QTabBar for each group of tabified docks and
// only ever fills in the tab text - there is no public API to reach those
// tab bars, so their icons stay empty. The accepted workaround (see e.g.
// QTBUG discussions and the Qt forums) is exactly this: find the direct
// QTabBar children, match each tab back to its dock widget by title, and
// stamp the dock's windowIcon on it. Titles are unique here (Libraries/
// Properties/FCD code), so the match is unambiguous.
void MainWindow::refreshDockTabIcons()
{
    const QList<QDockWidget *> docks = { ui->dockLibraries, ui->dockProperties, ui->dockFcdCode,
                                         ui->dockLayers };
    const QList<QTabBar *> tabBars = findChildren<QTabBar *>(QString(), Qt::FindDirectChildrenOnly);
    for (QTabBar *tabBar : tabBars) {
        for (int i = 0; i < tabBar->count(); ++i) {
            for (QDockWidget *dock : docks) {
                // themedIcon(): the tab bar is no QAction/QAbstractButton,
                // so the global re-tint pass never reaches it - black line
                // art would stay invisible on dark themes. Re-run from
                // applyLiveSettings() on a live theme switch.
                if (tabBar->tabText(i) == dock->windowTitle())
                    tabBar->setTabIcon(i, QIcon(ThemeManager::themedIcon(
                            dock->windowIcon().pixmap(20, 20))));
            }
        }
    }
}

QList<QAction *> MainWindow::toolbarActionCatalog() const
{
    // Pure view toggles make no sense as toolbar commands.
    static const QStringList Excluded = {
        QStringLiteral("actionToolBarBaseVisible"),
        QStringLiteral("actionToolBarPrimitiveVisible"),
        QStringLiteral("actionToolBarModifyVisible"),
        QStringLiteral("actionRulersVisible"),
        QStringLiteral("actionCustomizeToolbars"),
    };

    QList<QAction *> catalog;
    std::function<void(QMenu *)> collect = [&](QMenu *menu) {
        for (QAction *action : menu->actions()) {
            if (action->isSeparator())
                continue;
            if (QMenu *submenu = action->menu()) {
                collect(submenu);
                continue;
            }
            if (action->objectName().isEmpty() || action->text().isEmpty())
                continue; // dynamically built entries (recent files, dock toggles, ...)
            if (Excluded.contains(action->objectName()) || catalog.contains(action))
                continue;
            catalog.append(action);
        }
    };
    for (QAction *topLevel : ui->menubar->actions()) {
        if (QMenu *menu = topLevel->menu())
            collect(menu);
    }
    // Whatever already sits on a customizable toolbar belongs in the catalog
    // too, even if it never appears in a menu - otherwise the customize
    // dialog can't resolve (and silently drops) such a command from the
    // toolbar's own current layout, permanently losing its button on OK.
    for (QToolBar *toolBar : customizableToolbars()) {
        for (QAction *action : toolBar->actions()) {
            if (action->isSeparator() || qobject_cast<QWidgetAction *>(action))
                continue;
            if (action->objectName().isEmpty() || action->text().isEmpty())
                continue;
            if (!Excluded.contains(action->objectName()) && !catalog.contains(action))
                catalog.append(action);
        }
    }
    return catalog;
}

QList<QToolBar *> MainWindow::customizableToolbars() const
{
    return { ui->toolBarTools, ui->toolBarModify };
}

QStringList MainWindow::toolbarLayoutOf(QToolBar *toolBar) const
{
    QStringList layout;
    for (QAction *action : toolBar->actions()) {
        if (qobject_cast<QWidgetAction *>(action))
            continue; // e.g. the layer combobox - not a command, stays put
        if (action->isSeparator())
            layout.append(QStringLiteral("separator"));
        else if (!action->objectName().isEmpty())
            layout.append(action->objectName());
    }
    return layout;
}

void MainWindow::applyToolbarLayout(QToolBar *toolBar, const QStringList &layout)
{
    // Strip the current commands, keeping widget actions (layer combobox)
    // where they are; the rebuilt commands are inserted before the first of
    // them, i.e. exactly where the commands used to live.
    QAction *firstWidgetAction = nullptr;
    const QList<QAction *> existing = toolBar->actions();
    for (QAction *action : existing) {
        if (qobject_cast<QWidgetAction *>(action)) {
            if (!firstWidgetAction)
                firstWidgetAction = action;
            continue;
        }
        toolBar->removeAction(action);
        // Separators are throwaway QActions owned by the toolbar - delete
        // them or they'd pile up across rebuilds. Real commands are owned
        // by the .ui and just get detached.
        if (action->isSeparator() && action->parent() == toolBar)
            delete action;
    }

    for (const QString &name : layout) {
        if (name == QStringLiteral("separator")) {
            auto *separator = new QAction(toolBar);
            separator->setSeparator(true);
            toolBar->insertAction(firstWidgetAction, separator);
        } else if (QAction *action = findChild<QAction *>(name)) {
            toolBar->insertAction(firstWidgetAction, action);
        }
    }
}

void MainWindow::loadToolbarCustomizations()
{
    // "toolbar_custom2_": the original "toolbar_custom_" key is deliberately
    // ignored - layouts saved under it may have been silently stripped of
    // toolbar-only commands (the grid/snap toggles) by a customize dialog
    // whose catalog only knew menu commands, so they can't be trusted.
    for (QToolBar *toolBar : customizableToolbars()) {
        m_defaultToolbarLayouts.insert(toolBar, toolbarLayoutOf(toolBar));
        const QVariant saved = SettingsManager::getInstance()
                .loadSetting(QStringLiteral("toolbar_custom2_") + toolBar->objectName());
        if (saved.isValid())
            applyToolbarLayout(toolBar, saved.toStringList());
    }
}

void MainWindow::clickCustomizeToolbarsAction()
{
    QList<DialogCustomizeToolbars::ToolbarEntry> entries;
    // User-visible toolbar names: reuse the View menu's own toggle labels,
    // so the dialog and the menu can never drift apart.
    const QHash<QToolBar *, QString> titles = {
        { ui->toolBarTools, ui->actionToolBarBaseVisible->text() },
        { ui->toolBarModify, ui->actionToolBarModifyVisible->text() },
    };
    for (QToolBar *toolBar : customizableToolbars()) {
        DialogCustomizeToolbars::ToolbarEntry entry;
        entry.toolBar = toolBar;
        entry.title = titles.value(toolBar, toolBar->objectName());
        entry.currentLayout = toolbarLayoutOf(toolBar);
        entry.defaultLayout = m_defaultToolbarLayouts.value(toolBar);
        entries.append(entry);
    }

    DialogCustomizeToolbars dialog(toolbarActionCatalog(), entries, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QHash<QToolBar *, QStringList> layouts = dialog.layouts();
    for (auto it = layouts.constBegin(); it != layouts.constEnd(); ++it) {
        applyToolbarLayout(it.key(), it.value());
        SettingsManager::getInstance().saveSetting(
                QStringLiteral("toolbar_custom2_") + it.key()->objectName(), it.value());
    }
}

QList<QPair<QString, QAction *>> MainWindow::commandCatalogByCategory() const
{
    QList<QPair<QString, QAction *>> catalog;
    std::function<void(QMenu *, const QString &)> collect =
            [&](QMenu *menu, const QString &category) {
        for (QAction *action : menu->actions()) {
            if (action->isSeparator())
                continue;
            if (QMenu *submenu = action->menu()) {
                collect(submenu, category);
                continue;
            }
            // Dynamically built entries (recent files, ...) have no stable
            // objectName to key customizations on.
            if (action->objectName().isEmpty() || action->text().isEmpty())
                continue;
            catalog.append({ category, action });
        }
    };
    for (QAction *topLevel : ui->menubar->actions()) {
        if (QMenu *menu = topLevel->menu()) {
            QString category = topLevel->text();
            category.remove(QLatin1Char('&'));
            collect(menu, category);
        }
    }
    for (QAction *action : ui->toolBarPrimitive->actions()) {
        if (!action->isSeparator() && !action->objectName().isEmpty()
                && !action->text().isEmpty())
            catalog.append({ tr("Drawing tools"), action });
    }
    return catalog;
}

void MainWindow::loadShortcutCustomizations()
{
    const SettingsManager &settings = SettingsManager::getInstance();
    const auto catalog = commandCatalogByCategory();
    for (const auto &entry : catalog) {
        QAction *action = entry.second;
        m_defaultShortcuts.insert(action, action->shortcut());
        const QVariant saved = settings.loadSetting(
                QStringLiteral("shortcut_custom_") + action->objectName());
        if (saved.isValid())
            action->setShortcut(QKeySequence::fromString(saved.toString(),
                                                         QKeySequence::PortableText));
    }
}

// Every command reachable from the menu bar (grouped under its top-level
// menu's name) plus the drawing tools, handed to the palette as the very
// same QAction objects the menus use - so enabled state, icons and
// shortcuts always match, and triggering one is exactly a menu click.
void MainWindow::openCommandPalette(const QPoint &topCenter, const QString &initialText)
{
    CommandPalette palette(this);
    const auto catalog = commandCatalogByCategory();
    for (const auto &entry : catalog) {
        if (entry.second != ui->actionCommandPalette) // a pointless entry
            palette.addCommand(entry.first, entry.second);
    }
    palette.popup(topCenter, initialText);
}

void MainWindow::clickCommandPaletteAction()
{
    // Just under the menu bar, centered - where command palettes usually
    // drop down from.
    openCommandPalette(mapToGlobal(QPoint(width() / 2, menuBar()->height() + 4)));
}

void MainWindow::positionMenuBarSearch()
{
    if (!m_menuBarSearch)
        return;
    // Centered on the window's width, vertically centered in the bar -
    // never left of the menus though: on a narrow window the centered spot
    // would overlap them, so it gets pushed right just past the last menu.
    int menusRight = 0;
    const QList<QAction *> topLevel = ui->menubar->actions();
    if (!topLevel.isEmpty())
        menusRight = ui->menubar->actionGeometry(topLevel.last()).right();
    constexpr int Margin = 16;
    const int centered = (ui->menubar->width() - m_menuBarSearch->width()) / 2;
    const int fieldHeight = qMax(18, ui->menubar->height() - 6);
    m_menuBarSearch->setGeometry(qMax(centered, menusRight + Margin),
                                 (ui->menubar->height() - fieldHeight) / 2,
                                 m_menuBarSearch->width(), fieldHeight);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Keeps the search field glued after the menus whatever the bar does
    // (resize, first show, style/font change reflowing the menu widths).
    if (watched == ui->menubar
            && (event->type() == QEvent::Resize || event->type() == QEvent::Show
                || event->type() == QEvent::LayoutRequest)) {
        positionMenuBarSearch();
    }

    // New dock tab bars appear whenever document docks get grouped - wire
    // the close buttons and tear-off gesture onto each as it shows up. (The
    // side panels' tab bars are handled via their dock signals instead.)
    if (watched == m_documentArea
            && (event->type() == QEvent::ChildPolished || event->type() == QEvent::ChildAdded)) {
        setupDockTabBars();
        QTimer::singleShot(0, this, [this]() { updateDockTitleBars(); });
    }

    // (Tab dragging - reorder, tear-off, re-dock - is Qt's native
    // GroupedDragging behavior; nothing custom here.)

    // A document dock's title-bar X: run the unsaved-changes prompt and
    // actually remove the document, swallowing Qt's default hide-only close.
    if (event->type() == QEvent::Close) {
        if (Document *document = documentForObject(watched)) {
            closeDocument(document);
            return true;
        }
    }

    // Clicking or focusing a document's view makes it the active document -
    // "the one being worked on", which drives every panel and action. The
    // event is not consumed: it still reaches the view's own handling
    // (selection, placement) right after.
    if ((event->type() == QEvent::MouseButtonPress
         || event->type() == QEvent::FocusIn)) {
        if (Document *document = documentForObject(watched))
            setActiveDocument(document);
    }

    // The corner search field opens the palette on typing, not on focus -
    // see its textEdited connection in setConnections().
    return QMainWindow::eventFilter(watched, event);
}

QMenu *MainWindow::createPopupMenu()
{
    QMenu *menu = QMainWindow::createPopupMenu();
    if (menu) {
        menu->addSeparator();
        menu->addAction(ui->actionCustomizeToolbars);
    }
    return menu;
}

void MainWindow::applyLiveSettings()
{
    // A theme switch may just have happened (gui_style is a setting like
    // any other) - the dock tab icons are re-tinted per theme in there.
    refreshDockTabIcons();

    const int iconSizeSetting = SettingsManager::getInstance()
            .loadSetting("toolbar_icon_size").toInt();
    const int iconSize = iconSizeSetting > 0 ? qBound(16, iconSizeSetting, 64) : 25;
    for (QToolBar *toolBar : { static_cast<QToolBar *>(ui->toolBarTools),
                               static_cast<QToolBar *>(ui->toolBarModify),
                               static_cast<QToolBar *>(ui->toolBarPrimitive) })
        toolBar->setIconSize(QSize(iconSize, iconSize));

    // Toggles that mirror a setting editable from the Options dialog too -
    // signals blocked, or re-checking them would save the setting right
    // back and loop through settingIsChanged() forever.
    {
        const QSignalBlocker blocker(ui->actionBooleanSmooth);
        ui->actionBooleanSmooth->setChecked(
                SettingsManager::getInstance().loadSetting("boolean_smooth_results").toBool());
    }
    {
        const QVariant objectSnap = SettingsManager::getInstance().loadSetting("snap_objects");
        const bool enabled = objectSnap.isValid() ? objectSnap.toBool() : true;
        const QSignalBlocker blocker(ui->actionSnapToObjects);
        ui->actionSnapToObjects->setChecked(enabled);
        sheetScene->setObjectSnapEnabled(enabled);
    }

    // Macro tree icons are black line art rendered onto transparency - on
    // the dark themes they'd disappear against the dark panel, so re-derive
    // every row's icon for the active theme (ThemeManager::themedIcon()),
    // including after a live theme switch from the Options dialog. The pass
    // touches hundreds of rows, so it only runs when something it depends
    // on actually changed since the last pass.
    const QVariant treeIconsSetting = SettingsManager::getInstance()
            .loadSetting("macro_tree_icons_enabled");
    int macroIconSize = SettingsManager::getInstance().loadSetting("macro_icon_size").toInt();
    if (macroIconSize <= 0)
        macroIconSize = 32;
    static bool lastPassDark = false;
    static int lastPassIconSize = -1; // -1 = never ran
    const bool macroIconsWanted = (!treeIconsSetting.isValid() || treeIconsSetting.toBool())
            && ui->toolBoxLib->count() > 0;
    if (macroIconsWanted && (ThemeManager::darkThemeActive() != lastPassDark
                             || macroIconSize != lastPassIconSize)) {
        lastPassDark = ThemeManager::darkThemeActive();
        lastPassIconSize = macroIconSize;
        for (int page = 0; page < ui->toolBoxLib->count(); ++page) {
            auto *tree = qobject_cast<QTreeWidget *>(ui->toolBoxLib->widget(page));
            if (!tree)
                continue;
            for (int top = 0; top < tree->topLevelItemCount(); ++top) {
                QTreeWidgetItem *categoryItem = tree->topLevelItem(top);
                for (int child = 0; child < categoryItem->childCount(); ++child) {
                    QTreeWidgetItem *item = categoryItem->child(child);
                    const QString key = item->data(0, Qt::UserRole).toString();
                    if (!key.isEmpty())
                        item->setIcon(0, ThemeManager::themedIcon(
                                LibraryManager::getInstance().icon(key, macroIconSize)));
                }
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Every dirty document gets its own save/discard question, activated
    // first for visual context; a single Cancel aborts the whole close and
    // leaves everything open. Copied because confirmDiscardChanges() can
    // spin the event loop (save dialog), and a snapshot is simplest.
    const QList<Document *> documents = m_documents;
    for (Document *document : documents) {
        if (document->isClean())
            continue;
        setActiveDocument(document);
        if (document->dock)
            document->dock->raise();
        if (!confirmDiscardChanges()) {
            event->ignore();
            return;
        }
    }

    // Persists the current dock/toolbar arrangement (Libraries/Properties
    // panel position, size, floating, tabbing) so it's restored exactly
    // as left on the next launch - see the constructor's restoreState().
    SettingsManager::getInstance().saveSetting("window_dock_state", saveState(DockStateVersion));
    SettingsManager::getInstance().saveSetting("window_geometry", saveGeometry());
    // A clean, deliberate exit - nothing left to recover next launch.
    clearAllAutosaves();
    event->accept();
}

QString MainWindow::autosaveTargetPathFor(const Document *document) const
{
    if (!document->filePath().isEmpty()) {
        const QFileInfo info(document->filePath());
        return info.absolutePath() + QLatin1Char('/') + info.completeBaseName()
                + QStringLiteral(".autosave.fcd");
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    // Keyed by the untitled number, so several unsaved drawings each keep
    // their own sidecar.
    return dir + QStringLiteral("/untitled-%1.autosave.fcd")
            .arg(qMax(1, document->untitledNumber()));
}

QStringList MainWindow::pendingAutosaves()
{
    return SettingsManager::getInstance().loadSetting("pending_autosaves").toStringList();
}

void MainWindow::setPendingAutosaves(const QStringList &entries)
{
    SettingsManager::getInstance().saveSetting("pending_autosaves", entries);
}

void MainWindow::clearAutosaveFor(Document *document)
{
    if (!document)
        return;
    // Both the last sidecar actually written and the would-be-current one
    // (they differ right after a Save As moved the document elsewhere).
    QStringList sidecars { document->lastAutosavePath, autosaveTargetPathFor(document) };
    document->lastAutosavePath.clear();
    QStringList pending = pendingAutosaves();
    bool changed = false;
    for (const QString &sidecar : sidecars) {
        if (sidecar.isEmpty())
            continue;
        QFile::remove(sidecar);
        for (int i = pending.size() - 1; i >= 0; --i) {
            if (pending.at(i).section(QLatin1Char('|'), 0, 0) == sidecar) {
                pending.removeAt(i);
                changed = true;
            }
        }
    }
    if (changed)
        setPendingAutosaves(pending);
}

void MainWindow::clearAllAutosaves()
{
    for (const QString &entry : pendingAutosaves())
        QFile::remove(entry.section(QLatin1Char('|'), 0, 0));
    setPendingAutosaves(QStringList());
    // Legacy single-slot keys from pre-multidocument versions.
    const QString legacy = SettingsManager::getInstance()
            .loadSetting("pending_autosave_path").toString();
    if (!legacy.isEmpty())
        QFile::remove(legacy);
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
    // Every dirty document, active or not - a clean one is already safe
    // (never edited, or its last edit is on disk at its own path).
    QStringList pending = pendingAutosaves();
    bool changed = false;
    for (Document *document : std::as_const(m_documents)) {
        if (document->sheet()->undoStack()->isClean())
            continue;

        const QString path = autosaveTargetPathFor(document);
        QString error;
        // Best-effort: a failed autosave (e.g. a read-only directory)
        // shouldn't interrupt editing with an error dialog every few
        // minutes.
        if (!FidoCadWriter::writeFile(document->sheet(), path, &error))
            continue;

        document->lastAutosavePath = path;
        const QString entry = path + QLatin1Char('|') + document->filePath();
        if (!pending.contains(entry)) {
            // Drop any older entry for the same sidecar (e.g. the original
            // path changed) before recording the fresh one.
            for (int i = pending.size() - 1; i >= 0; --i) {
                if (pending.at(i).section(QLatin1Char('|'), 0, 0) == path)
                    pending.removeAt(i);
            }
            pending.append(entry);
            changed = true;
        }
    }
    if (changed)
        setPendingAutosaves(pending);
}

void MainWindow::checkForAutosaveRecovery()
{
    // Collect every recoverable entry: the multi-document registry, plus
    // the legacy single-slot keys a pre-multidocument version may have left.
    QStringList entries = pendingAutosaves();
    const QString legacyPath = SettingsManager::getInstance()
            .loadSetting("pending_autosave_path").toString();
    if (!legacyPath.isEmpty()) {
        entries.append(legacyPath + QLatin1Char('|') + SettingsManager::getInstance()
                       .loadSetting("pending_autosave_original").toString());
        SettingsManager::getInstance().saveSetting("pending_autosave_path", QString());
        SettingsManager::getInstance().saveSetting("pending_autosave_original", QString());
    }
    // Drop stale pointers (sidecars deleted by hand) up front.
    QStringList recoverable;
    QStringList names;
    for (const QString &entry : std::as_const(entries)) {
        const QString sidecar = entry.section(QLatin1Char('|'), 0, 0);
        if (!QFileInfo::exists(sidecar))
            continue;
        recoverable.append(entry);
        const QString original = entry.section(QLatin1Char('|'), 1);
        names.append(original.isEmpty() ? tr("an untitled drawing")
                                        : QFileInfo(original).fileName());
    }
    if (recoverable.isEmpty()) {
        setPendingAutosaves(QStringList());
        return;
    }

    const auto answer = QMessageBox::question(
                this, tr("Autosave recovery"),
                tr("eSchema wasn't closed properly last time.\n"
                   "Autosaves were found for:\n%1\n\n"
                   "Do you want to recover them?").arg(names.join(QLatin1Char('\n'))),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (answer != QMessageBox::Yes) {
        clearAllAutosaves();
        return;
    }

    for (const QString &entry : std::as_const(recoverable)) {
        const QString sidecar = entry.section(QLatin1Char('|'), 0, 0);
        const QString original = entry.section(QLatin1Char('|'), 1);

        // Each recovery lands in its own document: reuse the startup's
        // pristine untitled one first, then fresh tabs.
        if (!(currentFilePath.isEmpty() && sheetScene->primitives().isEmpty()
              && sheetScene->undoStack()->isClean()))
            createDocument();

        QString error;
        if (!FidoCadReader::readFile(sidecar, sheetScene, &error)) {
            QMessageBox::warning(this, tr("Error"),
                                  tr("Could not recover the autosave:\n%1").arg(error));
            continue;
        }
        normalizeLoadedDrawingPosition();
        if (!original.isEmpty())
            setCurrentFilePath(original);
        // Recovered content was never actually written to the real file (if
        // any) - readFile()'s bulk load leaves the stack clean like a normal
        // Open, so push a no-op command purely to flip isClean() to false.
        sheetScene->undoStack()->push(new QUndoCommand(tr("Autosave recovery")));
        updateDocumentTitle(m_activeDocument);
    }
    clearAllAutosaves();
}

void MainWindow::clickNewAction()
{
    // Multidocument: a new drawing simply opens in its own tab - whatever
    // was being worked on stays open next to it, nothing to confirm.
    createDocument();
}

void MainWindow::clickOpenAction()
{
    // No unsaved-changes prompt anymore: the file opens in its own tab (or
    // reuses a pristine untitled one) - see openFile().
    const QString path = QFileDialog::getOpenFileName(this, tr("Open drawing"), QString(),
                                                        tr("FidoCadJ (*.fcd)"));
    if (path.isEmpty())
        return;

    openFile(path);
}

// If the just-loaded drawing sticks out of the drawing area, shift it onto
// the sheet. One rigid translation for everything - primitives, macros,
// labels - by a whole number of units, so relative geometry and grid
// alignment are untouched; the view is then refitted so the user actually
// sees what was recovered. Deliberately not undoable (it's part of loading,
// like the parse itself), and it doesn't dirty the undo stack - the file on
// disk only changes if the user later saves.
void MainWindow::normalizeLoadedDrawingPosition()
{
    const QRectF bounds = sheetScene->itemsBoundingRect();
    if (bounds.isEmpty())
        return;
    const QRectF area = sheetScene->sceneRect();
    if (area.contains(bounds))
        return;

    // Pull the overflowing side back onto the sheet; the origin side wins
    // when the drawing is bigger than the sheet on an axis, keeping the
    // origin-side content visible.
    qreal dx = 0.0, dy = 0.0;
    if (bounds.right() > area.right())
        dx = area.right() - bounds.right();
    if (bounds.left() + dx < area.left())
        dx = area.left() - bounds.left();
    if (bounds.bottom() > area.bottom())
        dy = area.bottom() - bounds.bottom();
    if (bounds.top() + dy < area.top())
        dy = area.top() - bounds.top();

    // Whole units, rounded toward the inside of the sheet, so a file's
    // integer coordinates stay integer.
    dx = dx > 0 ? std::ceil(dx) : std::floor(dx);
    dy = dy > 0 ? std::ceil(dy) : std::floor(dy);
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy))
        return;

    const QPointF delta(dx, dy);
    for (GraphicsPrimitive *primitive : sheetScene->primitives())
        primitive->translateControlPoints(delta);

    activeView()->adjustView();
    ui->statusbar->showMessage(
            tr("The drawing had elements outside the drawing area and was moved onto the sheet"),
            6000);
}

bool MainWindow::openFile(const QString &filePath)
{
    // Already open? Just bring its dock forward instead of loading a second
    // copy that would silently compete for the same file on save.
    const QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    for (Document *document : std::as_const(m_documents)) {
        if (!document->filePath().isEmpty()
                && QFileInfo(document->filePath()).absoluteFilePath() == absolutePath) {
            if (document->dock)
                document->dock->raise();
            setActiveDocument(document);
            return true;
        }
    }

    // Reuse the active document while it's still a pristine untitled one
    // (the empty tab the app starts with); anything else keeps its tab and
    // the file opens in a new one.
    if (!(currentFilePath.isEmpty() && sheetScene->primitives().isEmpty()
          && sheetScene->undoStack()->isClean()))
        createDocument();

    // A placement in progress holds a raw pointer to its preview primitive,
    // which the bulk load's clearPrimitives() (QGraphicsScene::clear())
    // would destroy out from under it - the next mouse move would then
    // write through freed memory. Abandon the placement first.
    if (placementController)
        placementController->cancelPlacement();

    QString error;
    if (!FidoCadReader::readFile(filePath, sheetScene, &error)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to open the file:\n%1").arg(error));
        return false;
    }
    normalizeLoadedDrawingPosition();
    sheetScene->undoStack()->setClean();
    setCurrentFilePath(filePath);
    clearAutosaveFor(m_activeDocument);
    syncFcdCodeFromSheet(); // setClean() alone doesn't fire indexChanged()
    // The primitivesChanged hook covers non-empty files; loading an empty
    // one must still put the welcome card away.
    if (m_activeDocument && m_activeDocument->welcome)
        m_activeDocument->welcome->dismiss();
    return true;
}

void MainWindow::clickImportDxfAction()
{
    // No unsaved-changes prompt: the import lands in its own tab (or reuses
    // a pristine untitled one) - see importDxfFile().
    const QString path = QFileDialog::getOpenFileName(this, tr("Import from DXF"), QString(),
                                                        tr("DXF (*.dxf)"));
    if (path.isEmpty())
        return;

    importDxfFile(path);
}

bool MainWindow::importDxfFile(const QString &filePath)
{
    // Same tab policy as openFile(): reuse a pristine untitled document,
    // otherwise import into a brand-new tab.
    if (!(currentFilePath.isEmpty() && sheetScene->primitives().isEmpty()
          && sheetScene->undoStack()->isClean()))
        createDocument();

    // Same non-undoable bulk-load contract as Open/FidoCadReader::read().
    if (placementController)
        placementController->cancelPlacement(); // see openFile()
    QString error;
    QStringList warnings;
    if (!DxfReader::readFile(filePath, sheetScene, &error, &warnings)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to open the file:\n%1").arg(error));
        return false;
    }
    normalizeLoadedDrawingPosition();
    sheetScene->undoStack()->setClean();
    // A DXF file has no notion of "this eSchema document's own path" - it's
    // an import, not a native Open, so the next Save still goes through Save
    // As rather than silently overwriting the .dxf with .fcd content.
    setCurrentFilePath(QString());
    clearAutosaveFor(m_activeDocument);
    syncFcdCodeFromSheet(); // setClean() alone doesn't fire indexChanged()
    // Same as openFile(): an import into a fresh document must put the
    // welcome card away even when the DXF turned out empty.
    if (m_activeDocument && m_activeDocument->welcome)
        m_activeDocument->welcome->dismiss();

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
    // Serializing the whole document (tracing-image base64 included) and
    // re-laying-out the text box on *every* undo-stack change is pure waste
    // while the dock isn't even visible - it resyncs on visibilityChanged
    // instead (see setConnections()).
    if (!ui->dockFcdCode->isVisible())
        return;
    if (!ui->txtFcdCode->document()->isModified())
        syncFcdCodeFromSheet();
}

void MainWindow::clickRefreshFcdCodeAction()
{
    syncFcdCodeFromSheet();
}

void MainWindow::clickApplyFcdCodeAction()
{
    // Whatever placement was mid-flight would end up pushed/deleted as part
    // of the wholesale replacement below while the controller still holds
    // its pointer - abandon it first (see the same guard in openFile()).
    if (placementController)
        placementController->cancelPlacement();

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

QStringList MainWindow::droppableFilePaths(const QMimeData *mimeData)
{
    QStringList paths;
    if (!mimeData->hasUrls())
        return paths;
    // Every droppable file counts: each one opens in its own tab.
    for (const QUrl &url : mimeData->urls()) {
        const QString path = url.toLocalFile();
        if (path.endsWith(QStringLiteral(".fcd"), Qt::CaseInsensitive)
                || path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
            paths.append(path);
    }
    return paths;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!droppableFilePaths(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QStringList paths = droppableFilePaths(event->mimeData());
    if (paths.isEmpty())
        return;

    event->acceptProposedAction();
    // Each file lands in its own tab - nothing to confirm anymore.
    for (const QString &path : paths) {
        if (path.endsWith(QStringLiteral(".dxf"), Qt::CaseInsensitive))
            importDxfFile(path);
        else
            openFile(path);
    }
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
    m_printRealScale = optionsDialog.realScale();
    m_printScalePercent = optionsDialog.scalePercent();

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
        // Real scale instead prints dimensionally exact: one logical unit is
        // fixed at 1/200 inch (FidoCadJ's unit), so at 100% the printed size
        // is resolution/200 device pixels per unit - whatever doesn't fit
        // the page is simply clipped, since scale accuracy is the point.
        const qreal scale = m_printRealScale
                ? printer->resolution() / 200.0 * (m_printScalePercent / 100.0)
                : qMin(targetRect.width() / sourceRect.width(),
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
