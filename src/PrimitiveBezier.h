/*
 * PrimitiveBezier.h
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

#ifndef PRIMITIVEBEZIER_H
#define PRIMITIVEBEZIER_H

#include "GraphicsPrimitive.h"

// FCD "BE" - a cubic Bezier curve: 2 endpoints + 2 control points
// (FIDOSPECS.md 5.2). Control point order matches toTokens(): p1, ctrl1, ctrl2, p2.
class PrimitiveBezier : public GraphicsPrimitive
{
public:
    explicit PrimitiveBezier(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 4; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsArrows() const override { return true; }

private:
    QPointF m_points[4];
};

#endif // PRIMITIVEBEZIER_H
