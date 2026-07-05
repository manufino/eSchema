#include "RotatePrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

RotatePrimitiveCommand::RotatePrimitiveCommand(GraphicsPrimitive *primitive, const QPointF &pivot)
    : m_primitive(primitive), m_pivot(pivot)
{
    setText(QObject::tr("Ruota"));
}

void RotatePrimitiveCommand::undo()
{
    // 3 more quarter-turns = 4 total from the pre-redo state = identity.
    m_primitive->rotate90(m_pivot);
    m_primitive->rotate90(m_pivot);
    m_primitive->rotate90(m_pivot);
}

void RotatePrimitiveCommand::redo()
{
    m_primitive->rotate90(m_pivot);
}
