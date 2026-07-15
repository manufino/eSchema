/*
 * PrimitiveRectangle.h
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

#ifndef PRIMITIVERECTANGLE_H
#define PRIMITIVERECTANGLE_H

#include "GraphicsPrimitive.h"

// FCD "RV" (void/outline) / "RP" (painted/filled) - an axis-aligned rectangle
// between two opposite corners; void vs. filled is the existing `filled` flag on
// the base class (FIDOSPECS.md 5.3).
class PrimitiveRectangle : public GraphicsPrimitive
{
public:
    explicit PrimitiveRectangle(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 2; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    bool supportsBooleanOps() const override { return true; }
    QPainterPath booleanOutline() const override;

    // Editable size (Properties panel): the normalized top-left corner stays
    // fixed and the opposite one moves.
    qreal shapeWidth() const;
    qreal shapeHeight() const;
    void setShapeSize(qreal width, qreal height);

    bool isDegenerate() const override;
    QStringList toTokens() const override;

private:
    QPointF m_p1, m_p2;
};

#endif // PRIMITIVERECTANGLE_H
