#include "PrimitiveConnection.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitiveConnection::PrimitiveConnection(QGraphicsItem *parent)
    : GraphicsPrimitive(Connection, parent)
{
}

QRectF PrimitiveConnection::boundingRect() const
{
    const qreal r = m_diameter / 2.0 + 1;
    return QRectF(mapFromScene(m_pos) - QPointF(r, r), QSizeF(2 * r, 2 * r));
}

void PrimitiveConnection::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    const QColor color = objLayer ? objLayer->color() : QColor(Qt::black);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);

    const qreal r = m_diameter / 2.0;
    painter->drawEllipse(mapFromScene(m_pos), r, r);
}

QPointF PrimitiveConnection::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveConnection::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

QStringList PrimitiveConnection::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("SA"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(layerIndex())
    };
}
