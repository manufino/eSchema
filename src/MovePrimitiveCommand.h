#ifndef MOVEPRIMITIVECOMMAND_H
#define MOVEPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QVector>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for dragging a primitive's body (Select tool). Stores absolute
// control-point snapshots from before/after the drag rather than a relative
// delta: the drag already applied the "after" state live for visual feedback,
// so redo() must be safe to call again on push() without moving it twice.
class MovePrimitiveCommand : public QUndoCommand
{
public:
    MovePrimitiveCommand(GraphicsPrimitive *primitive, const QVector<QPointF> &before,
                          const QVector<QPointF> &after);

    void undo() override;
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    QVector<QPointF> m_before;
    QVector<QPointF> m_after;
};

#endif // MOVEPRIMITIVECOMMAND_H
