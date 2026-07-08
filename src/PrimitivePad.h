/*
 * PrimitivePad.h
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

#ifndef PRIMITIVEPAD_H
#define PRIMITIVEPAD_H

#include "GraphicsPrimitive.h"

// FCD "PA" - a PCB pad centered at a point, with outer size, an internal hole
// diameter, and a shape style (FIDOSPECS.md 5.9). No FCJ; name/value TY labels
// may follow directly.
class PrimitivePad : public GraphicsPrimitive
{
public:
    enum Style { Round = 0, Rectangular = 1, RoundedRectangular = 2 };

    explicit PrimitivePad(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    qreal outerWidth() const { return m_rx; }
    qreal outerHeight() const { return m_ry; }
    qreal holeDiameter() const { return m_ri; }
    Style style() const { return m_style; }
    void setOuterSize(qreal rx, qreal ry) { m_rx = rx; m_ry = ry; }
    void setHoleDiameter(qreal ri) { m_ri = ri; }
    void setStyle(Style style) { m_style = style; }

    bool isDegenerate() const override { return false; } // a pad is always meaningful
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    QPainterPath buildPath() const;

    QPointF m_pos;
    qreal m_rx = 10.0, m_ry = 10.0, m_ri = 5.0;
    Style m_style = Round;
};

#endif // PRIMITIVEPAD_H
