#ifndef ROTATEPRIMITIVECOMMAND_H
#define ROTATEPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for Edit > Rotate (a fixed 90 degree turn). redo() applies one
// rotate90(); since 4 applications return to the original state (for both
// the rotated points and any extra orientation field a subclass like
// PrimitiveMacro/PrimitiveText advances mod 4/360), undo() applies 3 more,
// landing back where it started without needing a separate "before" snapshot.
class RotatePrimitiveCommand : public QUndoCommand
{
public:
    RotatePrimitiveCommand(GraphicsPrimitive *primitive, const QPointF &pivot);

    void undo() override;
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    QPointF m_pivot;
};

#endif // ROTATEPRIMITIVECOMMAND_H
