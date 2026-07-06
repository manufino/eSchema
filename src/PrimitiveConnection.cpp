/*
 * PrimitiveConnection.cpp
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

#include "PrimitiveConnection.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitiveConnection::PrimitiveConnection(QGraphicsItem *parent)
    : GraphicsPrimitive(Connection, parent)
{
}

QRectF PrimitiveConnection::boundingRect() const
{
    const qreal r = m_diameter / 2.0 + 1;
    return QRectF(mapFromScene(m_pos) - QPointF(r, r), QSizeF(2 * r, 2 * r))
            .united(labelBoundingRect());
}

void PrimitiveConnection::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    const QColor color = drawColor();
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);

    const qreal r = m_diameter / 2.0;
    painter->drawEllipse(mapFromScene(m_pos), r, r);

    paintLabels(painter);
}

QPointF PrimitiveConnection::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveConnection::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

QStringList PrimitiveConnection::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("SA"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(layerIndex())
    };
}
