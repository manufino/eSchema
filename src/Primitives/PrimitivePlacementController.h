/*
 * PrimitivePlacementController.h
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

#ifndef PRIMITIVEPLACEMENTCONTROLLER_H
#define PRIMITIVEPLACEMENTCONTROLLER_H

#include <QObject>
#include <QPointF>
#include <QLineF>
#include <QRectF>
#include <QString>

class Sheet;
class SheetView;
class ToolBarPrimitive;
class LayerComboBox;
class QCheckBox;
class QKeyEvent;
class QAction;
class GraphicsPrimitive;

// Drives interactive primitive *creation*. Reads which drawing tool is checked
// on the toolbar and turns a sequence of scene clicks into a new primitive
// added to the Sheet. One click-driven state machine handles both
// fixed-point-count tools (line/rectangle/ellipse/bezier/connection/text/
// pcbtrack/pad) and variable-vertex tools (polygon/complex curve) uniformly,
// via GraphicsPrimitive's control-point interface.
//
// Note: every primitive type that has a toolbar action is creatable
// interactively (Line, Rectangle, Polygon, Ellipse, Bezier, Curve, Text,
// Connection, PcbTrack, Pad). Image has no placement tool at all (matching
// the reference FidoCadJ editor, which places it from an "insert image"
// dialog), so it remains data-model-only until read from a FidoCadJ file.
// Macro has no toolbar action either - it's armed from the macro library
// panel instead (see armMacroPlacement()) - but is otherwise placed through
// the very same one-click state machine as Connection/Pad/Text.
class PrimitivePlacementController : public QObject
{
    Q_OBJECT
public:
    PrimitivePlacementController(SheetView *view, Sheet *sheet, ToolBarPrimitive *toolBar,
                                 LayerComboBox *layerCombo, QCheckBox *fillCheckBox,
                                 QObject *parent = nullptr);

    // True when a placement tool (anything but "Select") is checked, or a
    // macro is armed - SheetView uses this to skip its normal
    // selection/pan handling while placing.
    bool isPlacementToolActive() const;

    // The primitive currently being placed (nullptr when none) - SheetView
    // passes it to Sheet::snapPosition() as excluded, so an in-progress
    // shape's own preview vertices never become object-snap targets.
    GraphicsPrimitive *activePrimitive() const { return m_activePrimitive; }

    // Called by SheetView from its own mouse event overrides with an
    // already-grid-snapped scene position. Return value: true if consumed.
    bool handleMousePress(const QPointF &scenePos);
    bool handleMouseMove(const QPointF &scenePos);
    bool handleMouseDoubleClick(const QPointF &scenePos);
    // Right-click ends a Line/PCB-track chain (see isChainableTool()'s doc
    // comment) without leaving the tool. Returns false (unconsumed) for
    // every other tool/state, matching FidoCadJ where right-click has no
    // effect outside of ending that specific chain.
    bool handleMouseRightClick();
    bool handleKeyPress(QKeyEvent *event);

    // Abandons whatever placement is in progress (removing the preview
    // primitive and any dimension extras from the sheet) and returns the
    // toolbar to Select. Public because every document-replacing operation
    // (New/Open/DXF import/Apply FCD code) must call it before clearing the
    // scene: the controller's raw preview pointers would otherwise dangle.
    void cancelPlacement();

    // Arms a single macro instance for placement: the next click on the
    // sheet drops it there (like Connection/Pad/Text - a single-click tool,
    // no chaining), after which placement returns to Select, exactly as any
    // other one-click tool already does. Called from the library panel
    // instead of a toolbar action, since macros have no fixed toolbar slot.
    // Cancels whatever toolbar-driven placement was in progress, if any.
    void armMacroPlacement(const QString &macroKey);

private slots:
    // Whenever the toolbar's own checked tool changes - including the user
    // clicking the already-visible Select button directly while mid-draw,
    // not just via Escape - abandons whatever was in progress instead of
    // leaving an orphaned, never-finished primitive sitting in the document.
    // The toolbar's checked state is left alone: it already reflects
    // whichever tool the user just picked, Select or otherwise.
    void handleToolBarActionTriggered(QAction *action);

signals:
    // Live readout of the Measure tool (distance/angle between its two
    // points) - MainWindow routes it to the status bar. Emitted on every
    // mouse move while measuring and once more, final, at the second click.
    void measureUpdated(const QString &text);

private:
    enum class Tool { Select, Line, Rectangle, Polygon, RegularPolygon, Ellipse, Bezier, Curve, Arc,
                      Text, Connection, PcbTrack, Pad, Macro, Image, Measure, Dimension,
                      DimensionAngular, DimensionRadial };

    Tool currentTool() const;
    int requiredPointCount(Tool tool) const; // -1 means variable vertex count
    void startPlacement(const QPointF &scenePos);
    void finishPlacement();
    // Removes the in-progress primitive (if any) from the sheet and resets
    // the point-count/active-primitive state, without touching the toolbar
    // or m_activeTool - shared by cancelPlacement() (which also does both of
    // those) and handleToolBarActionTriggered() (which must not, since the
    // toolbar's checked state at that point is exactly what the user just
    // picked).
    void discardActivePrimitive();
    bool isVariableVertexTool(Tool tool) const;
    // The point placement should actually use for `scenePos` while Shift is
    // held: for line-like segments (Line/PCB track chains, polygon/curve
    // edges) the direction from the last fixed point is locked to the
    // nearest multiple of 45°; for Rectangle/Ellipse the second corner is
    // forced square, so Shift drags out perfect squares/circles - the
    // standard CAD/vector-editor gesture. Returns `scenePos` unchanged when
    // Shift is up or the active tool/state has no anchor to constrain
    // against.
    QPointF constrainedPlacementPoint(const QPointF &scenePos) const;
    // Line and PCB track are the only two FidoCadJ tools that "chain":
    // finishing one segment immediately begins the next one from the same
    // endpoint, with no extra click needed for its first point - matching
    // ElementsEdtActions.addLine()/addPCBLine() in the reference FidoCadJ
    // editor, which reuse xpoly[1]/ypoly[1] from the point just placed
    // instead of resetting clickNumber to 0. Every other multi-click tool
    // (Bezier, Polygon, Complex curve, Rectangle, Ellipse) ends cleanly and
    // requires a fresh, independent click sequence for its next instance.
    bool isChainableTool(Tool tool) const;
    // Shared primitive-construction step behind both startPlacement() (first
    // segment/shape of a tool) and startChainedSegment() (every subsequent
    // chained Line/PCB-track segment) - so layer/fill defaults are applied
    // identically either way.
    GraphicsPrimitive *createPrimitiveForTool(Tool tool) const;
    void applyDefaults(GraphicsPrimitive *primitive) const;
    // Begins the next chained segment right where the previous one ended -
    // no click consumed, just a live "point 1" preview like any other
    // in-progress placement.
    void startChainedSegment(Tool tool, const QPointF &startPoint);
    // Opens the "insert image" file picker immediately when the Immagine
    // toolbar button is clicked (rather than waiting for a canvas click
    // first, like every other tool) - see handleToolBarActionTriggered().
    // On success, creates and adds the PrimitiveImage right away, centered
    // on the view for now; handleMouseMove() takes over from there and
    // keeps it centered on the cursor, at its already-fixed size, until the
    // next click drops it in place. Reverts to Select on cancel or read
    // failure, matching every other placement that can be aborted.
    void armImagePlacement();
    // Programmatically switches the toolbar back to the Select tool, exactly
    // as if the user had clicked/pressed its shortcut - so Esc while mid-
    // placement (see handleKeyPress()) doesn't just discard the in-progress
    // primitive but actually leaves drawing mode, matching the reference
    // FidoCadJ editor (PopUpMenu's Escape binding goes to the same
    // "selection" action as pressing A).
    void switchToolBarToSelectTool();

    SheetView *m_view;
    Sheet *m_sheet;
    ToolBarPrimitive *m_toolBar;
    LayerComboBox *m_layerCombo;
    QCheckBox *m_fillCheckBox;

    GraphicsPrimitive *m_activePrimitive = nullptr;
    Tool m_activeTool = Tool::Select;
    int m_pointsPlaced = 0;
    // Set by armMacroPlacement(), consumed by the next startPlacement() -
    // empty means no macro is armed. Kept separate from m_activeTool/
    // currentTool() because there's no toolbar action driving it.
    QString m_armedMacroKey;
    // Half the width/height an image armed by armImagePlacement() keeps as
    // it follows the cursor - fixed once at arm time (from the picked
    // file's own pixel size, scaled to a sane on-sheet footprint), not
    // resized by the placement click the way Rectangle's second corner is.
    QPointF m_imageHalfSize;
    // The Arc tool's two fixed endpoints (clicks 1 and 2). The third click -
    // a point the arc passes through - never becomes a vertex itself: it
    // just picks the circumcircle, and the whole curve is resampled along
    // that arc (see the Tool::Arc branches in handleMousePress/Move).
    QPointF m_arcStart;
    QPointF m_arcEnd;
    // Regular polygon tool: center fixed by the first click; the cursor is
    // the first vertex (radius and orientation both follow it) until the
    // second click. Side count is asked once per placement, remembering the
    // last choice ("regular_polygon_sides").
    void updateRegularPolygonVertices(const QPointF &vertexPos);
    QPointF m_regularPolygonCenter;
    int m_regularPolygonSides = 6;
    // Measure tool: m_activePrimitive is a dashed rubber line that never
    // reaches the undo stack - the second click removes it again and leaves
    // only the status-bar readout (see measureUpdated()).
    QString measureText(const QPointF &from, const QPointF &to) const;
    QPointF m_measureStart;

    // Dimension tool: two clicks fix the measured points, the third places
    // the dimension line parallel to them at the cursor's offset. The
    // annotation is plain primitives (a double-arrowed line, two extension
    // lines, a text with the distance), so the file format is untouched.
    // m_activePrimitive is the dimension line itself; the extension lines
    // and the label live in m_dimensionExtras while previewing, and
    // everything is pushed onto the undo stack as one macro at the final
    // click. The measured distance is stated in millimeters and the label
    // sized per "dimension_text_size" (Options > Drawing).
    void updateDimensionPreview(const QPointF &offsetPoint);
    QString dimensionLabel(qreal length) const;
    QPointF m_dimensionStart;
    QPointF m_dimensionEnd;
    QList<GraphicsPrimitive *> m_dimensionExtras;

    // A distance/angle label primitive configured per the dimension
    // settings (size, default font) - shared by the linear, angular and
    // radial dimension tools.
    GraphicsPrimitive *createDimensionLabel() const;
    // Shared tail of every dimension tool's final click: pushes the live
    // preview primitives (m_activePrimitive + m_dimensionExtras) onto the
    // undo stack as one `undoLabel` macro and resets the state, staying
    // armed for the next annotation.
    void finishDimensionAnnotation(const QString &undoLabel);
    // Angular dimension, AutoCAD-style: pick two existing lines (any
    // straight segment - lines, PCB tracks, rectangle and polygon edges),
    // each highlighted on hover, then the third click places the measuring
    // arc at the cursor's radius, on the cursor's side. The vertex is the
    // lines' (extended) intersection; each ray aims at where its segment
    // was clicked. m_activePrimitive is the arc (an open complex curve with
    // arrows); the label lives in m_dimensionExtras.
    void updateAngularDimensionPreview(const QPointF &cursor);
    QPointF m_angleVertex;
    QPointF m_angleFirst;
    QPointF m_angleSecond;
    QLineF m_angleSegmentA;  // the first picked segment...
    QPointF m_angleClickA;   // ...and where on it the user clicked
    // Radial dimension, single click: hovering highlights the circle/arc
    // outline under the cursor; clicking it drops a radius arrow from the
    // automatically found center to the picked rim point, measure included.
    void updateRadialDimensionPreview(const QPointF &cursor);
    QPointF m_radialCenter;

    // --- Hover picking shared by the two tools above -----------------------
    // The cursor's raw (unsnapped) scene position - picks must never jump
    // to the grid or an object-snap point, or the segment under the mouse
    // could stop being under the pick.
    QPointF cursorScenePos() const;
    struct SegmentPick {
        QLineF segment;
        QPointF clicked;
        bool valid = false;
    };
    // The straight segment (line/track/rectangle edge/polygon edge) within
    // a few screen pixels of `scenePos`, nearest first.
    SegmentPick segmentNear(const QPointF &scenePos) const;
    struct CirclePick {
        QPointF center;
        QPointF rim;    // point on the outline nearest the cursor
        qreal radius = 0.0;
        QRectF rect;    // what to highlight (the ellipse/circle outline)
        bool valid = false;
    };
    // The circle-like outline near `scenePos`: an ellipse's perimeter, or
    // a complex curve's circumcircle (an Arc-tool arc), center included.
    CirclePick circleNear(const QPointF &scenePos) const;
    // Refreshes the Sheet's hover highlight per the active tool and state -
    // called from handleMouseMove(), including while the tool is merely
    // armed with nothing placed yet.
    void updatePickHighlight();

    // The pre-tooltip body of handleMouseMove() - split out so the public
    // wrapper can refresh the tooltip once, after whichever of the many
    // per-tool branches ran, instead of before every return.
    bool handleMouseMoveImpl(const QPointF &rawScenePos);
    // Dynamic cursor tooltip ("dynamic_tooltip" setting, Options > Drawing):
    // refreshes the Sheet's placement tooltip with the live geometry of
    // whatever is being drawn - segment length and angle, rectangle/ellipse
    // width x height, regular polygon and arc radius - or hides it for the
    // tools with nothing measurable (text, connection, macro drops...).
    // Called at the end of every handleMouseMove() with the same
    // constrained position the preview itself just used.
    void updatePlacementTooltip(const QPointF &scenePos);
    // The length text per the status bar's units preference (units, mm, or
    // both - "units_display"), shared by the tooltip's formats.
    QString tooltipLength(qreal length) const;
};

#endif // PRIMITIVEPLACEMENTCONTROLLER_H
