/*
 * PrimitiveComplexCurve.h
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

#ifndef PRIMITIVECOMPLEXCURVE_H
#define PRIMITIVECOMPLEXCURVE_H

#include "GraphicsPrimitive.h"
#include <QVector>
#include <QPainterPath>

// FCD "CV" (void/outline) / "CP" (painted/filled) - a smooth curve interpolating
// an arbitrary number of control points, optionally closed (FIDOSPECS.md 5.6).
// Arrows are only meaningful when the curve is open.
class PrimitiveComplexCurve : public GraphicsPrimitive
{
public:
    static constexpr int MaxVertices = 10000;

    explicit PrimitiveComplexCurve(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return m_vertices.size(); }
    QPointF controlPoint(int index) const override { return m_vertices.at(index); }
    void setControlPoint(int index, const QPointF &scenePos) override;

    void appendVertex(const QPointF &scenePos);
    void removeLastVertex();
    int vertexCount() const { return m_vertices.size(); }

    bool supportsNodeEditing() const override { return true; }
    void insertControlPoint(int index, const QPointF &scenePos) override;
    void removeControlPointAt(int index) override;
    bool isClosedShape() const override { return m_closed; }

    bool isClosed() const { return m_closed; }
    // prepareGeometryChange()/update(): can now be changed live from the
    // Properties panel - closing the curve changes both its fill/shape and
    // whether its (now-irrelevant) arrows are still painted.
    void setClosed(bool closed) { prepareGeometryChange(); m_closed = closed; update(); }

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsArrows() const override { return !m_closed; }

private:
    QPainterPath buildSplinePath() const;

    QVector<QPointF> m_vertices;
    bool m_closed = false;
};

#endif // PRIMITIVECOMPLEXCURVE_H
