#include "PrimitivePad.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitivePad::PrimitivePad(QGraphicsItem *parent)
    : GraphicsPrimitive(Pad, parent)
{
}

QRectF PrimitivePad::boundingRect() const
{
    const qreal margin = 1;
    return QRectF(mapFromScene(m_pos) - QPointF(m_rx / 2, m_ry / 2), QSizeF(m_rx, m_ry))
            .adjusted(-margin, -margin, margin, margin);
}

void PrimitivePad::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    const QColor color = objLayer ? objLayer->color() : QColor(Qt::black);
    const QPointF center = mapFromScene(m_pos);
    const QRectF outer(center - QPointF(m_rx / 2, m_ry / 2), QSizeF(m_rx, m_ry));

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    switch (m_style) {
    case Round:
        painter->drawEllipse(outer);
        break;
    case Rectangular:
        painter->drawRect(outer);
        break;
    case RoundedRectangular:
        painter->drawRoundedRect(outer, outer.width() * 0.2, outer.height() * 0.2);
        break;
    }

    // Punch the drill hole out of the copper pad by clearing (not drawing) the
    // pixels underneath it, so it reads correctly regardless of the sheet's
    // background color.
    if (m_ri > 0) {
        painter->setCompositionMode(QPainter::CompositionMode_Clear);
        painter->drawEllipse(center, m_ri / 2, m_ri / 2);
    }
    painter->restore();
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
