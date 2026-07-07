/*
 * PrimitiveLine.cpp
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
    const qreal margin = effectiveLineWidth() + 4; // room for the pen width and any arrowhead
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitiveLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(drawColor());
    pen.setStyle(penStyle);
    pen.setWidthF(effectiveLineWidth());
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

    paintLabels(painter);
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
