/*
 * SheetView.h
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

#ifndef SHEETVIEW_H
#define SHEETVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGraphicsRectItem>
#include <QColor>
#include <QEvent>
#include <QContextMenuEvent>

#include "Sheet.h"
#include "SettingsManager.h"
#include "GlobalUtils.h"

class PrimitivePlacementController;

#define ZOOM_SCALE_MIN 0.3// Scala minima consentita
#define ZOOM_SCALE_MAX 15.0// Scala massima consentita

// The interactive viewport onto a Sheet: draws the grid/background/tracing
// image behind the scene and the guides on top of it, and turns raw mouse
// input into the app's gestures - wheel zoom, guide dragging, placement
// clicks forwarded to the PrimitivePlacementController, the AutoCAD-style
// window/crossing rubber band, and the context-menu request MainWindow
// answers. Holds no Sheet* of its own (it reads scene() each time), so any
// number of views can exist, one per open document (see DocumentView).
class SheetView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit SheetView(QWidget *parent = nullptr);
    // Overrides the grid spacing/color loaded from settings - used by the
    // print/export paths that render through a temporary view.
    void setGrid(int size, QColor clr);

    // Not owned - the controller is created and owned by MainWindow, which
    // wires it up here since it needs sibling widgets (toolbar, property
    // panel) that SheetView itself has no knowledge of.
    void setPlacementController(PrimitivePlacementController *controller) { m_placementController = controller; }

    // Rounds a scene position to the nearest multiple of the configured
    // snap step (SettingsManager "snap_step"), or returns it unchanged when
    // "snap_enabled" is off. One scene unit == one FidoCadJ grid unit, so the
    // default step of 1 guarantees integer coordinates (FIDOSPECS.md 3).
    QPointF snapToGrid(const QPointF &scenePos) const;

    // The position a placement click/preview at `viewPos` should land on:
    // Sheet::snapPosition() (object snap when enabled, grid otherwise),
    // excluding whatever the placement controller is currently building.
    QPointF placementSnap(const QPoint &viewPos);

    // Scene-unit spacing between consecutive minor/major grid lines, exactly
    // as drawn by drawBackground() - RulerWidget reads these so its ticks
    // land on the same lines as the visible grid instead of using an
    // independently chosen spacing.
    qreal minorGridStep() const { return qMax(1, gridSize); }
    qreal majorGridStep() const { return qMax(1, gridMarkSize / 10) * qMax(1, gridSize); }

protected:
    // Flat background color, then the tracing image, then the grid (dots/
    // lines/both per the "grid_type" setting) - all view-side, so exports
    // and prints (which render the Sheet directly) never include them.
    void drawBackground (QPainter* painter, const QRectF &rect);
    // Base implementation first (which forwards to Sheet::drawForeground -
    // pad holes, snap indicator), then the sheet's guide lines on top.
    // Guides are drawn here, on the view side, deliberately: Sheet::render()
    // (print/export) never calls the view's hooks, so guides can never leak
    // onto paper or into an exported file.
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    // Paints the sheet's tracing reference image (if any), between the flat
    // background fill and the grid lines - a no-op when none is set. Split
    // out since drawBackground() has an early-return path when the grid is
    // off that still needs this to run.
    void drawTracingImage(QPainter *painter, const QRectF &rect);
    // Ctrl+wheel (or plain wheel per settings) zooms in integer levels
    // around the cursor, clamped to [ZOOM_SCALE_MIN, ZOOM_SCALE_MAX];
    // otherwise scrolls normally.
    void wheelEvent(QWheelEvent *event);
    // Drives, in order: guide dragging, the placement preview (forwarded to
    // the controller with the snapped position), the rubber-band update,
    // and the mouseMoved()/mousePosChanged() signals the status bar and
    // rulers track.
    void mouseMoveEvent(QMouseEvent *event);
    // Left press: forwards a placement click to the controller, grabs a
    // guide under the cursor, or - on empty canvas with the Select tool -
    // starts the window/crossing rubber band. Right press while placing
    // cancels the placement and switches back to the Select tool (the
    // same click's contextMenuEvent then opens the menu).
    void mousePressEvent(QMouseEvent* event);
    // Ends the active gesture: drops (or deletes, if released outside the
    // viewport) a dragged guide, or resolves the rubber band into a
    // selection - left-to-right selects only fully contained items,
    // right-to-left also touched ones; Ctrl/Shift extends.
    void mouseReleaseEvent(QMouseEvent *event);
    // Offers the double-click to the placement controller first (it ends
    // multi-point chains like polylines); otherwise falls through to the
    // items, e.g. a text primitive opening its edit dialog.
    void mouseDoubleClickEvent(QMouseEvent *event);
    // Offers the key to the placement controller first (Escape cancels the
    // in-progress placement); a bare Escape otherwise clears the selection.
    void keyPressEvent(QKeyEvent *event);
    // Right-clicking a not-yet-selected primitive replaces the selection with
    // just that one (so the menu MainWindow builds in response to
    // contextMenuRequested() applies to what was actually clicked) before
    // MainWindow is asked to show it; right-clicking one already part of a
    // multi-selection, or empty canvas, leaves the current selection as-is.
    void contextMenuEvent(QContextMenuEvent *event) override;
    // Both fire viewTransformChanged() (see below): a resize can shift what
    // scene point sits under viewport pixel (0,0) since resizeAnchor
    // defaults to AnchorViewCenter, and the mouse leaving is when the
    // rulers' tracking marker should disappear.
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *event) override;
    // Macro drag & drop from the Libraries panel (see MacroLibraryTree in
    // MainWindowLibraryPanel.cpp, the drag source): drags carrying the
    // custom macro mime type are accepted here and land as macroDropped();
    // anything else (e.g. .fcd/.dxf files) is ignored ON PURPOSE so the
    // event propagates up to MainWindow's window-wide file-drop handling
    // instead of being swallowed by the scene.
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // Reads every grid/color/zoom-related setting into the members below.
    void loadSettings();
    // Applies the current zoomLevel to the view transform and emits
    // zoomScaleIsChanged()/viewTransformChanged().
    void zoomUpdate();
    // The guide within grab distance (a few screen pixels) of `viewPos`, or
    // -1 - see Sheet::guideNear().
    int guideIndexAt(const QPoint &viewPos) const;
    // The guide's new position for the cursor at `viewPos`, grid-snapped on
    // its own axis (via the shared snap-to-grid helper) so dropped guides
    // land on round coordinates.
    qreal guidePositionFor(const Sheet::Guide &guide, const QPoint &viewPos) const;

public:
    // The current zoom level as the percentage the status bar displays
    // (100% = ZOOM_SCALE_MAX, so the range is 2-100).
    int zoomPercent() const { return zoomLevel; }

public slots:
    // Jumps to an exact zoom percentage (same scale convention as
    // zoomPercent()), anchored at the viewport center - the status bar's
    // zoom menu drives this. Clamped to the wheel-zoom limits.
    void setZoomPercent(int percent);
    // Connected to SettingsManager::settingIsChanged - re-reads the settings
    // and repaints, so Options changes show up live.
    void settingChanged();
    // Fit-to-drawing: frames the bounding box of all primitives (with a
    // margin), or resets to the default zoom on an empty sheet.
    void adjustView();
    // Fits the view to the bounding box of the current selection - a no-op
    // when nothing is selected, so it never surprises the user by silently
    // falling back to fit-all.
    void adjustViewToSelection();

signals:
    // Cursor position in scene coordinates, on every move - the status
    // bar's coordinate readout.
    void mouseMoved(QPointF point);
    // New zoom level in percent - the status bar's zoom display.
    void zoomScaleIsChanged(unsigned int level);
    // Companion to mouseMoved() for listeners that only need "the cursor
    // moved" (the rulers' tracking marker reads the position itself).
    void mousePosChanged();
    // MainWindow builds and execs the actual QMenu (it owns the ui->action*
    // objects the menu reuses) - SheetView only decides where/on-what the
    // click landed. scenePos is also passed through for context-dependent
    // entries that need where on the drawing the click landed (e.g. add/
    // remove node on a polygon/complex curve), not just where to pop up.
    void contextMenuRequested(const QPoint &globalPos, const QPointF &scenePos);
    // Fired whenever pan, zoom, or resize changes the mapping between scene
    // and viewport coordinates - MainWindow reads transform()/mapFromScene()
    // in response to this to keep the rulers' tick spacing and origin in
    // sync with the view.
    void viewTransformChanged();
    // The mouse pointer has left the viewport - MainWindow uses this to hide
    // the rulers' tracking marker, since no further mouseMoved() will arrive.
    void mouseLeftView();
    // A macro dragged from the Libraries panel was dropped here. `scenePos`
    // is already snapped (same placementSnap() rule as a placement click);
    // MainWindow creates the PrimitiveMacro and pushes the undo command.
    void macroDropped(const QString &macroKey, const QPointF &scenePos);

private:
    int m_originX, m_originY, gridSize, gridMarkSize, zoomLevel;
    float lineGridWidth, lineThickGridWidth;
    bool gridEnabled;
    QColor lineGridColor, lineThickGridColor, dotsGridColor, backgroundColor;
    QPoint point;
    Utils::GridType gridType;
    PrimitivePlacementController *m_placementController = nullptr;
    // Index (into Sheet::guides()) of the guide a left-drag is currently
    // moving, -1 when none - releasing it outside the viewport deletes it.
    int m_draggedGuide = -1;
    // Hand-made rubber-band selection with AutoCAD window/crossing
    // semantics (see the constructor's dragMode comment). Coordinates are
    // viewport pixels; the polygon is mapped to the scene at paint/release.
    bool m_rubberActive = false;
    QPoint m_rubberOrigin;
    QPoint m_rubberCurrent;
};

#endif // SHEETVIEW_H
