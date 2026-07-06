/*
 * PrimitiveBezier.cpp
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

#include "PrimitiveBezier.h"
#include "PrimitiveArrowUtils.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

PrimitiveBezier::PrimitiveBezier(QGraphicsItem *parent)
    : GraphicsPrimitive(Bezier, parent)
{
}

QRectF PrimitiveBezier::boundingRect() const
{
    QPolygonF poly;
    for (const QPointF &p : m_points)
        poly << mapFromScene(p);
    const qreal margin = penSize + 4;
    return poly.boundingRect().adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitiveBezier::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(drawColor());
    pen.setStyle(penStyle);
    pen.setWidthF(penSize);
    painter->setPen(pen);

    const QPointF p1 = mapFromScene(m_points[0]);
    const QPointF c1 = mapFromScene(m_points[1]);
    const QPointF c2 = mapFromScene(m_points[2]);
    const QPointF p2 = mapFromScene(m_points[3]);

    QPainterPath path(p1);
    path.cubicTo(c1, c2, p2);
    painter->drawPath(path);

    if (arrowAtStart())
        PrimitiveArrowUtils::paintArrow(painter, p1, c1, arrowStyleLimiter(), arrowStyleEmpty(),
                                        arrowLength(), arrowHalfWidth());
    if (arrowAtEnd())
        PrimitiveArrowUtils::paintArrow(painter, p2, c2, arrowStyleLimiter(), arrowStyleEmpty(),
                                        arrowLength(), arrowHalfWidth());

    paintLabels(painter);
}

QPointF PrimitiveBezier::controlPoint(int index) const
{
    return m_points[index];
}

void PrimitiveBezier::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_points[index] = scenePos;
}

bool PrimitiveBezier::isDegenerate() const
{
    return m_points[0] == m_points[3] && m_points[1] == m_points[3] && m_points[2] == m_points[3]
            && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitiveBezier::toTokens() const
{
    using namespace FidoCadTokenUtils;
    QStringList tokens { QStringLiteral("BE") };
    for (const QPointF &p : m_points) {
        tokens << roundIntelligently(p.x()) << roundIntelligently(p.y());
    }
    tokens << QString::number(layerIndex());
    return tokens;
}
