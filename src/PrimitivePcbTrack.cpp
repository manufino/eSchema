/*
 * PrimitivePcbTrack.cpp
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

#include "PrimitivePcbTrack.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

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

QPainterPath PrimitivePcbTrack::shape() const
{
    QPainterPath path(mapFromScene(m_p1));
    path.lineTo(mapFromScene(m_p2));
    return withLabelArea(strokeOutline(path, m_width));
}

void PrimitivePcbTrack::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(drawColor());
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
