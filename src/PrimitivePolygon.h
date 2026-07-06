/*
 * PrimitivePolygon.h
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

#ifndef PRIMITIVEPOLYGON_H
#define PRIMITIVEPOLYGON_H

#include "GraphicsPrimitive.h"
#include <QVector>

// FCD "PV" (void/outline) / "PP" (painted/filled) - a polygon with an arbitrary
// number of vertices, capped at MAX_VERTICES (FIDOSPECS.md 5.5).
class PrimitivePolygon : public GraphicsPrimitive
{
public:
    static constexpr int MaxVertices = 10000;

    explicit PrimitivePolygon(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return m_vertices.size(); }
    QPointF controlPoint(int index) const override { return m_vertices.at(index); }
    void setControlPoint(int index, const QPointF &scenePos) override;

    // Used while interactively placing the polygon (one click = one vertex).
    void appendVertex(const QPointF &scenePos);
    void removeLastVertex();
    int vertexCount() const { return m_vertices.size(); }

    bool isDegenerate() const override;
    QStringList toTokens() const override;

private:
    QVector<QPointF> m_vertices;
};

#endif // PRIMITIVEPOLYGON_H
