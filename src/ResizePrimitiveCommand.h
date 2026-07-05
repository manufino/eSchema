#ifndef RESIZEPRIMITIVECOMMAND_H
#define RESIZEPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for dragging a single resize handle. Like MovePrimitiveCommand,
// stores absolute before/after positions (the drag already applied "after"
// live), so redo() is safe to call again when the command is pushed.
class ResizePrimitiveCommand : public QUndoCommand
{
public:
    ResizePrimitiveCommand(GraphicsPrimitive *primitive, int controlPointIndex,
                            const QPointF &before, const QPointF &after);

    void undo() override;
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    int m_index;
    QPointF m_before;
    QPointF m_after;
};

#endif // RESIZEPRIMITIVECOMMAND_H
