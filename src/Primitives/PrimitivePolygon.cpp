/*
 * PrimitivePolygon.cpp
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

#include "PrimitivePolygon.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

PrimitivePolygon::PrimitivePolygon(QGraphicsItem *parent)
    : GraphicsPrimitive(Polyline, parent)
{
}

QRectF PrimitivePolygon::boundingRect() const
{
    QPolygonF poly;
    for (const QPointF &p : m_vertices)
        poly << mapFromScene(p);
    const qreal margin = effectiveLineWidth() + 2;
    return poly.boundingRect().adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

QPainterPath PrimitivePolygon::shape() const
{
    if (m_vertices.size() < 2)
        return QPainterPath();

    QPolygonF poly;
    for (const QPointF &p : m_vertices)
        poly << mapFromScene(p);
    QPainterPath path;
    path.addPolygon(poly);
    path.closeSubpath();
    const QPainterPath outline = strokeOutline(path, effectiveLineWidth());
    if (isFilled())
        return withLabelArea(path.united(outline));
    return withLabelArea(outline);
}

void PrimitivePolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_vertices.size() < 2)
        return;

    QPen pen(drawColor());
    pen.setStyle(penStyle);
    pen.setWidthF(effectiveLineWidth());
    painter->setPen(pen);
    painter->setBrush(isFilled() ? QBrush(pen.color()) : QBrush(Qt::NoBrush));

    QPolygonF poly;
    for (const QPointF &p : m_vertices)
        poly << mapFromScene(p);
    painter->drawPolygon(poly);

    paintLabels(painter);
}

void PrimitivePolygon::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_vertices[index] = scenePos;
}

void PrimitivePolygon::appendVertex(const QPointF &scenePos)
{
    if (m_vertices.size() >= MaxVertices)
        return;
    prepareGeometryChange();
    m_vertices.append(scenePos);
}

void PrimitivePolygon::removeLastVertex()
{
    if (m_vertices.isEmpty())
        return;
    prepareGeometryChange();
    m_vertices.removeLast();
}

void PrimitivePolygon::insertControlPoint(int index, const QPointF &scenePos)
{
    if (m_vertices.size() >= MaxVertices)
        return;
    prepareGeometryChange();
    m_vertices.insert(index, scenePos);
}

void PrimitivePolygon::removeControlPointAt(int index)
{
    prepareGeometryChange();
    m_vertices.remove(index);
}

QPainterPath PrimitivePolygon::booleanOutline() const
{
    QPainterPath path;
    if (m_vertices.size() < 3)
        return path;
    path.addPolygon(QPolygonF(m_vertices));
    path.closeSubpath();
    return path;
}

bool PrimitivePolygon::isDegenerate() const
{
    return m_vertices.size() < 2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitivePolygon::toTokens() const
{
    using namespace FidoCadTokenUtils;
    QStringList tokens { isFilled() ? QStringLiteral("PP") : QStringLiteral("PV") };
    for (const QPointF &p : m_vertices)
        tokens << roundIntelligently(p.x()) << roundIntelligently(p.y());
    tokens << QString::number(layerIndex());
    return tokens;
}
