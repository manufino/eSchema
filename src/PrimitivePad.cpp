#include "PrimitivePad.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

PrimitivePad::PrimitivePad(QGraphicsItem *parent)
    : GraphicsPrimitive(Pad, parent)
{
}

QRectF PrimitivePad::boundingRect() const
{
    const qreal margin = 1;
    return QRectF(mapFromScene(m_pos) - QPointF(m_rx / 2, m_ry / 2), QSizeF(m_rx, m_ry))
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitivePad::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    const QColor color = objLayer ? objLayer->color() : QColor(Qt::black);
    const QPointF center = mapFromScene(m_pos);
    const QRectF outer(center - QPointF(m_rx / 2, m_ry / 2), QSizeF(m_rx, m_ry));

    // Built as a single even-odd path (outer shape + inner hole) rather than
    // drawing the hole with CompositionMode_Clear: clearing to transparent
    // doesn't composite correctly over the view's opaque backing store (the
    // "hole" came out solid black instead of see-through). The even-odd fill
    // rule punches the hole directly, with no compositing involved.
    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    switch (m_style) {
    case Round:
        path.addEllipse(outer);
        break;
    case Rectangular:
        path.addRect(outer);
        break;
    case RoundedRectangular:
        path.addRoundedRect(outer, outer.width() * 0.2, outer.height() * 0.2);
        break;
    }
    if (m_ri > 0)
        path.addEllipse(center, m_ri / 2, m_ri / 2);

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPath(path);

    paintLabels(painter);
}

QPointF PrimitivePad::controlPoint(int) const
{
    return m_pos;
}

void PrimitivePad::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

QStringList PrimitivePad::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("PA"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        roundIntelligently(m_rx), roundIntelligently(m_ry), roundIntelligently(m_ri),
        QString::number(int(m_style)),
        QString::number(layerIndex())
    };
}
