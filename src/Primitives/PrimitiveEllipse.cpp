/*
 * PrimitiveEllipse.cpp
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

#include "PrimitiveEllipse.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

PrimitiveEllipse::PrimitiveEllipse(QGraphicsItem *parent)
    : GraphicsPrimitive(Ellipse, parent)
{
}

QRectF PrimitiveEllipse::boundingRect() const
{
    const qreal margin = effectiveLineWidth() + 2;
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

QPainterPath PrimitiveEllipse::shape() const
{
    QPainterPath path;
    path.addEllipse(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized());
    const QPainterPath outline = strokeOutline(path, effectiveLineWidth());
    if (isFilled())
        return withLabelArea(path.united(outline));
    return withLabelArea(outline);
}

void PrimitiveEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    QPen pen(drawColor());
    pen.setWidthF(effectiveLineWidth());
    applyLineStyle(pen);
    painter->setPen(pen);
    painter->setBrush(isFilled() ? QBrush(pen.color()) : QBrush(Qt::NoBrush));

    painter->drawEllipse(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized());

    paintLabels(painter);
}

QPointF PrimitiveEllipse::controlPoint(int index) const
{
    return index == 0 ? m_p1 : m_p2;
}

void PrimitiveEllipse::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    if (index == 0)
        m_p1 = scenePos;
    else
        m_p2 = scenePos;
}

qreal PrimitiveEllipse::shapeWidth() const
{
    return QRectF(m_p1, m_p2).normalized().width();
}

qreal PrimitiveEllipse::shapeHeight() const
{
    return QRectF(m_p1, m_p2).normalized().height();
}

void PrimitiveEllipse::setShapeSize(qreal width, qreal height)
{
    QRectF rect = QRectF(m_p1, m_p2).normalized();
    rect.setSize(QSizeF(width, height));
    prepareGeometryChange();
    m_p1 = rect.topLeft();
    m_p2 = rect.bottomRight();
    update();
}

QPainterPath PrimitiveEllipse::booleanOutline() const
{
    QPainterPath path;
    path.addEllipse(QRectF(m_p1, m_p2).normalized());
    return path;
}

bool PrimitiveEllipse::isDegenerate() const
{
    return m_p1 == m_p2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitiveEllipse::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        isFilled() ? QStringLiteral("EP") : QStringLiteral("EV"),
        roundIntelligently(m_p1.x()), roundIntelligently(m_p1.y()),
        roundIntelligently(m_p2.x()), roundIntelligently(m_p2.y()),
        QString::number(layerIndex())
    };
}
