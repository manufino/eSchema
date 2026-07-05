#include "MirrorPrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

MirrorPrimitiveCommand::MirrorPrimitiveCommand(GraphicsPrimitive *primitive, Qt::Orientation axis,
                                                const QPointF &pivot)
    : m_primitive(primitive), m_axis(axis), m_pivot(pivot)
{
    setText(QObject::tr("Specchia"));
}

void MirrorPrimitiveCommand::undo()
{
    m_primitive->mirror(m_axis, m_pivot);
}

void MirrorPrimitiveCommand::redo()
{
    m_primitive->mirror(m_axis, m_pivot);
}
