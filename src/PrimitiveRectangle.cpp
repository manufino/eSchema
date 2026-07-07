/*
 * PrimitiveRectangle.cpp
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "PrimitiveRectangle.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitiveRectangle::PrimitiveRectangle(QGraphicsItem *parent)
    : GraphicsPrimitive(Rectangle, parent)
{
}

QRectF PrimitiveRectangle::boundingRect() const
{
    const qreal margin = effectiveLineWidth() + 2;
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitiveRectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(drawColor());
    pen.setStyle(penStyle);
    pen.setWidthF(effectiveLineWidth());
    painter->setPen(pen);
    painter->setBrush(isFilled() ? QBrush(pen.color()) : QBrush(Qt::NoBrush));

    painter->drawRect(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized());

    paintLabels(painter);
}

QPointF PrimitiveRectangle::controlPoint(int index) const
{
    return index == 0 ? m_p1 : m_p2;
}

void PrimitiveRectangle::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    if (index == 0)
        m_p1 = scenePos;
    else
        m_p2 = scenePos;
}

bool PrimitiveRectangle::isDegenerate() const
{
    return m_p1 == m_p2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitiveRectangle::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        isFilled() ? QStringLiteral("RP") : QStringLiteral("RV"),
        roundIntelligently(m_p1.x()), roundIntelligently(m_p1.y()),
        roundIntelligently(m_p2.x()), roundIntelligently(m_p2.y()),
        QString::number(layerIndex())
    };
}
