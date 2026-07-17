/*
 * PrimitiveArrowUtils.h
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

#ifndef PRIMITIVEARROWUTILS_H
#define PRIMITIVEARROWUTILS_H

#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QPainterPath>
#include <cmath>

// Shared arrowhead painter for the 3 primitives that support FCJ arrows
// (PrimitiveLine, PrimitiveBezier, PrimitiveComplexCurve) - kept as one free
// function instead of copy-pasting the triangle/limiter-bar drawing 3 times.
namespace PrimitiveArrowUtils {

// Paints an arrowhead at `tip`, pointing away from `tail` (i.e. the arrow
// points in the tail->tip direction), using the given FCJ arrow style bits,
// exactly like the reference FidoCadJ editor's Arrow.drawArrow():
// - the triangle is ALWAYS drawn - filled normally, outline-only with the
//   empty style (the two style bits combine, they are not alternatives;
//   drawing only the bar for the limiter style was wrong);
// - the limiter style ADDS a short bar perpendicular to the line at the
//   tip (the classic dimension-line terminator), on top of the triangle.
// Returns the triangle's base point (tip pulled back by `length`): the
// caller shortens its stroke to there when length > 0, again matching the
// reference (PrimitiveLine.draw()'s issue-#172 handling), so the line
// never pokes through an empty arrowhead.
inline QPointF paintArrow(QPainter *painter, const QPointF &tip, const QPointF &tail,
                          bool limiterStyle, bool emptyStyle, qreal length, qreal halfWidth)
{
    QPointF dir = tip - tail;
    const qreal len = std::hypot(dir.x(), dir.y());
    if (len < 1e-6)
        return tip;
    dir /= len; // unit vector from tail to tip
    const QPointF normal(-dir.y(), dir.x());

    const QPointF base = tip - dir * length;
    const QPointF left = base + normal * halfWidth;
    const QPointF right = base - normal * halfWidth;

    QPolygonF triangle;
    triangle << tip << left << right;

    if (emptyStyle) {
        painter->drawPolygon(triangle);
    } else {
        QPainterPath path;
        path.addPolygon(triangle);
        painter->fillPath(path, painter->pen().color());
    }

    if (limiterStyle) {
        const QPointF a = tip + normal * halfWidth;
        const QPointF b = tip - normal * halfWidth;
        painter->drawLine(a, b);
    }

    return length > 0 ? base : tip;
}

} // namespace PrimitiveArrowUtils

#endif // PRIMITIVEARROWUTILS_H
