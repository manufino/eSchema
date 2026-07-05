#include "ResizePrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

ResizePrimitiveCommand::ResizePrimitiveCommand(GraphicsPrimitive *primitive, int controlPointIndex,
                                                const QPointF &before, const QPointF &after)
    : m_primitive(primitive), m_index(controlPointIndex), m_before(before), m_after(after)
{
    setText(QObject::tr("Ridimensiona"));
}

void ResizePrimitiveCommand::undo()
{
    m_primitive->setControlPoint(m_index, m_before);
}

void ResizePrimitiveCommand::redo()
{
    m_primitive->setControlPoint(m_index, m_after);
}
