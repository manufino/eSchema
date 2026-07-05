#include "PrimitiveText.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QFontMetricsF>

PrimitiveText::PrimitiveText(QGraphicsItem *parent)
    : GraphicsPrimitive(Text, parent)
{
}

QRectF PrimitiveText::boundingRect() const
{
    QFont font(m_fontName);
    font.setPointSizeF(qMax(1, m_sizeX));
    const QFontMetricsF metrics(font);
    const QRectF textRect = metrics.boundingRect(m_text);
    return QRectF(mapFromScene(m_pos), textRect.size()).adjusted(-2, -2, 2, 2);
}

void PrimitiveText::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_text.isEmpty())
        return;

    QFont font(m_fontName);
    font.setPointSizeF(qMax(1, m_sizeX));
    font.setBold(m_styleFlags & Bold);
    font.setItalic(m_styleFlags & Italic);
    painter->setFont(font);
    painter->setPen(objLayer ? objLayer->color() : QColor(Qt::black));

    painter->save();
    painter->translate(mapFromScene(m_pos));
    painter->rotate(-m_orientationDeg); // FCD orientation is counter-clockwise
    if (m_styleFlags & Mirrored)
        painter->scale(-1, 1);
    painter->drawText(QPointF(0, 0), m_text);
    painter->restore();
}

QPointF PrimitiveText::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveText::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

QStringList PrimitiveText::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("TY"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(m_sizeY), QString::number(m_sizeX),
        QString::number(m_orientationDeg), QString::number(m_styleFlags),
        QString::number(layerIndex()),
        encodeFontName(m_fontName),
        m_text
    };
}
