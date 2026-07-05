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
// fixed-point-count tools (line/rectangle/ellipse/bezier/connection/text) and
// variable-vertex tools (polygon/complex curve) uniformly, via
// GraphicsPrimitive's control-point interface.
//
// Note: only the primitive types that already have a toolbar action are
// creatable interactively (Line, Rectangle, Polygon, Ellipse, Bezier, Curve,
// Text, Connection). PcbTrack/Pad/Macro/Image have no toolbar entry point yet,
// so they remain data-model-only until drawn via a future toolbar addition or
// read from a FidoCadJ file.
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
    bool handleKeyPress(QKeyEvent *event);

private:
    enum class Tool { Select, Line, Rectangle, Polygon, Ellipse, Bezier, Curve, Text, Connection };

    Tool currentTool() const;
    int requiredPointCount(Tool tool) const; // -1 means variable vertex count
    void startPlacement(const QPointF &scenePos);
    void finishPlacement();
    void cancelPlacement();
    bool isVariableVertexTool(Tool tool) const;

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
