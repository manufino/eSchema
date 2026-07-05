#include "MovePrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

MovePrimitiveCommand::MovePrimitiveCommand(GraphicsPrimitive *primitive, const QVector<QPointF> &before,
                                            const QVector<QPointF> &after)
    : m_primitive(primitive), m_before(before), m_after(after)
{
    setText(QObject::tr("Sposta"));
}

void MovePrimitiveCommand::undo()
{
    m_primitive->restoreControlPoints(m_before);
}

void MovePrimitiveCommand::redo()
{
    m_primitive->restoreControlPoints(m_after);
}
