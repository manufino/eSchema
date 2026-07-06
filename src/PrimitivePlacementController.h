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

class Sheet;
class SheetView;
class ToolBarPrimitive;
class LayerComboBox;
class QCheckBox;
class QKeyEvent;
class GraphicsPrimitive;

// Drives interactive primitive *creation*. Reads which drawing tool is checked
// on the toolbar and turns a sequence of scene clicks into a new primitive
// added to the Sheet. One click-driven state machine handles both
// fixed-point-count tools (line/rectangle/ellipse/bezier/connection/text/
// pcbtrack/pad) and variable-vertex tools (polygon/complex curve) uniformly,
// via GraphicsPrimitive's control-point interface.
//
// Note: only the primitive types that already have a toolbar action are
// creatable interactively (Line, Rectangle, Polygon, Ellipse, Bezier, Curve,
// Text, Connection, PcbTrack, Pad). Macro/Image have no toolbar entry point
// (matching the reference FidoCadJ editor, which places them from the macro
// library panel / an "insert image" dialog rather than a keyboard-shortcut
// drawing tool), so they remain data-model-only until read from a FidoCadJ
// file.
class PrimitivePlacementController : public QObject
{
    Q_OBJECT
public:
    PrimitivePlacementController(SheetView *view, Sheet *sheet, ToolBarPrimitive *toolBar,
                                 LayerComboBox *layerCombo, QCheckBox *fillCheckBox,
                                 QObject *parent = nullptr);

    // True when a placement tool (anything but "Select") is checked - SheetView
    // uses this to skip its normal selection/pan handling while placing.
    bool isPlacementToolActive() const;

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

private:
    enum class Tool { Select, Line, Rectangle, Polygon, Ellipse, Bezier, Curve, Text, Connection,
                      PcbTrack, Pad };

    Tool currentTool() const;
    int requiredPointCount(Tool tool) const; // -1 means variable vertex count
    void startPlacement(const QPointF &scenePos);
    void finishPlacement();
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
};

#endif // PRIMITIVEPLACEMENTCONTROLLER_H
