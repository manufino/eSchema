#ifndef MIRRORPRIMITIVECOMMAND_H
#define MIRRORPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for Edit > Mirror. Unlike Move/Resize, this isn't applied live
// before the command is pushed - redo() (auto-called once by
// QUndoStack::push()) is the only place GraphicsPrimitive::mirror() is
// actually invoked. Mirroring twice around the same axis/pivot is an
// identity operation (for both the reflected points and any extra
// orientation/mirror field a subclass like PrimitiveMacro/PrimitiveText
// toggles), so undo() just calls mirror() again instead of needing its own
// separate "before" state.
class MirrorPrimitiveCommand : public QUndoCommand
{
public:
    MirrorPrimitiveCommand(GraphicsPrimitive *primitive, Qt::Orientation axis, const QPointF &pivot);

    void undo() override;
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    Qt::Orientation m_axis;
    QPointF m_pivot;
};

#endif // MIRRORPRIMITIVECOMMAND_H
