#include "PrimitiveLine.h"
#include "PrimitiveArrowUtils.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitiveLine::PrimitiveLine(QGraphicsItem *parent)
    : GraphicsPrimitive(Line, parent)
{
}

QRectF PrimitiveLine::boundingRect() const
{
    const qreal margin = penSize + 4; // room for the pen width and any arrowhead
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .adjusted(-margin, -margin, margin, margin);
}

void PrimitiveLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(objLayer ? objLayer->color() : QColor(Qt::black));
    pen.setStyle(penStyle);
    pen.setWidthF(penSize);
    painter->setPen(pen);

    const QPointF p1 = mapFromScene(m_p1);
    const QPointF p2 = mapFromScene(m_p2);
    painter->drawLine(p1, p2);

    if (arrowAtStart())
        PrimitiveArrowUtils::paintArrow(painter, p1, p2, arrowStyleLimiter(), arrowStyleEmpty(),
                                        arrowLength(), arrowHalfWidth());
    if (arrowAtEnd())
        PrimitiveArrowUtils::paintArrow(painter, p2, p1, arrowStyleLimiter(), arrowStyleEmpty(),
                                        arrowLength(), arrowHalfWidth());
}

QPointF PrimitiveLine::controlPoint(int index) const
{
    return index == 0 ? m_p1 : m_p2;
}

void PrimitiveLine::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    if (index == 0)
        m_p1 = scenePos;
    else
        m_p2 = scenePos;
}

bool PrimitiveLine::isDegenerate() const
{
    // A zero-length line with no label carries no information (FIDOSPECS.md 5.1/9.7).
    return m_p1 == m_p2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitiveLine::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("LI"),
        roundIntelligently(m_p1.x()), roundIntelligently(m_p1.y()),
        roundIntelligently(m_p2.x()), roundIntelligently(m_p2.y()),
        QString::number(layerIndex())
    };
}
