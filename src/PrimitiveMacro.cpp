#include "PrimitiveMacro.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

namespace {
// No library is loaded, so a placeholder box is drawn instead of the real macro
// body - just big enough to show the key text.
constexpr qreal PlaceholderSize = 40.0;
}

PrimitiveMacro::PrimitiveMacro(QGraphicsItem *parent)
    : GraphicsPrimitive(PartLib, parent)
{
}

QRectF PrimitiveMacro::boundingRect() const
{
    const qreal half = PlaceholderSize / 2;
    return QRectF(mapFromScene(m_pos) - QPointF(half, half), QSizeF(PlaceholderSize, PlaceholderSize))
            .united(labelBoundingRect());
}

void PrimitiveMacro::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(drawColor());
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    const QPointF center = mapFromScene(m_pos);
    const qreal half = PlaceholderSize / 2;
    const QRectF box(center - QPointF(half, half), QSizeF(PlaceholderSize, PlaceholderSize));
    painter->drawRect(box);
    painter->drawText(box, Qt::AlignCenter | Qt::TextWordWrap, m_macroName);

    paintLabels(painter);
}

QPointF PrimitiveMacro::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveMacro::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

void PrimitiveMacro::mirror(Qt::Orientation, const QPointF &)
{
    prepareGeometryChange();
    m_mirrored = !m_mirrored;
}

void PrimitiveMacro::rotate90(const QPointF &)
{
    prepareGeometryChange();
    m_orientation = (m_orientation + 1) % 4;
}

QStringList PrimitiveMacro::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("MC"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(m_orientation), m_mirrored ? QStringLiteral("1") : QStringLiteral("0"),
        m_macroName
    };
}
