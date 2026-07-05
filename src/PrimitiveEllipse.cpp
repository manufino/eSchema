#include "PrimitiveEllipse.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitiveEllipse::PrimitiveEllipse(QGraphicsItem *parent)
    : GraphicsPrimitive(Ellipse, parent)
{
}

QRectF PrimitiveEllipse::boundingRect() const
{
    const qreal margin = penSize + 2;
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitiveEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(objLayer ? objLayer->color() : QColor(Qt::black));
    pen.setStyle(penStyle);
    pen.setWidthF(penSize);
    painter->setPen(pen);
    painter->setBrush(isFilled() ? QBrush(pen.color()) : QBrush(Qt::NoBrush));

    painter->drawEllipse(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized());

    paintLabels(painter);
}

QPointF PrimitiveEllipse::controlPoint(int index) const
{
    return index == 0 ? m_p1 : m_p2;
}

void PrimitiveEllipse::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    if (index == 0)
        m_p1 = scenePos;
    else
        m_p2 = scenePos;
}

bool PrimitiveEllipse::isDegenerate() const
{
    return m_p1 == m_p2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitiveEllipse::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        isFilled() ? QStringLiteral("EP") : QStringLiteral("EV"),
        roundIntelligently(m_p1.x()), roundIntelligently(m_p1.y()),
        roundIntelligently(m_p2.x()), roundIntelligently(m_p2.y()),
        QString::number(layerIndex())
    };
}
