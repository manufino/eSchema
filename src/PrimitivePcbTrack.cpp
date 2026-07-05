#include "PrimitivePcbTrack.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitivePcbTrack::PrimitivePcbTrack(QGraphicsItem *parent)
    : GraphicsPrimitive(PcbTrack, parent)
{
}

QRectF PrimitivePcbTrack::boundingRect() const
{
    const qreal margin = m_width / 2.0 + 1;
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitivePcbTrack::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(objLayer ? objLayer->color() : QColor(Qt::black));
    pen.setWidthF(m_width);
    pen.setCapStyle(Qt::RoundCap); // PCB tracks have rounded ends, like real copper
    painter->setPen(pen);

    painter->drawLine(mapFromScene(m_p1), mapFromScene(m_p2));

    paintLabels(painter);
}

QPointF PrimitivePcbTrack::controlPoint(int index) const
{
    return index == 0 ? m_p1 : m_p2;
}

void PrimitivePcbTrack::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    if (index == 0)
        m_p1 = scenePos;
    else
        m_p2 = scenePos;
}

bool PrimitivePcbTrack::isDegenerate() const
{
    return m_p1 == m_p2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitivePcbTrack::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("PL"),
        roundIntelligently(m_p1.x()), roundIntelligently(m_p1.y()),
        roundIntelligently(m_p2.x()), roundIntelligently(m_p2.y()),
        roundIntelligently(m_width),
        QString::number(layerIndex())
    };
}
