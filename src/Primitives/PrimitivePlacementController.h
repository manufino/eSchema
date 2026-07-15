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

private:
    enum class Tool { Select, Line, Rectangle, Polygon, RegularPolygon, Ellipse, Bezier, Curve, Arc,
                      Text, Connection, PcbTrack, Pad, Macro, Image };

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
    void cancelPlacement();
    bool isVariableVertexTool(Tool tool) const;
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
};

#endif // PRIMITIVEPLACEMENTCONTROLLER_H
