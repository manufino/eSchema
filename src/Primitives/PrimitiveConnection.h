/*
 * PrimitiveConnection.h
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

#ifndef PRIMITIVECONNECTION_H
#define PRIMITIVECONNECTION_H

#include "GraphicsPrimitive.h"

// FCD "SA" - a connection point (filled dot), drawn at the document's
// `diameterConnection` size (default 2.0, overridable per-document via "FJC C",
// FIDOSPECS.md 5.7/7). No FCJ line; name/value TY labels may follow directly.
class PrimitiveConnection : public GraphicsPrimitive
{
public:
    explicit PrimitiveConnection(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    qreal diameter() const { return m_diameter; }
    void setDiameter(qreal diameter) { m_diameter = diameter; }

    bool isDegenerate() const override { return false; } // a connection dot is always meaningful
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    QPointF m_pos;
    qreal m_diameter = 2.0;
};

#endif // PRIMITIVECONNECTION_H
