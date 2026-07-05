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

// Paints an arrowhead at `tip`, pointing away from `tail` (i.e. the arrow points
// in the tail->tip direction), using the given FCJ arrow style bits.
inline void paintArrow(QPainter *painter, const QPointF &tip, const QPointF &tail,
                       bool limiterStyle, bool emptyStyle, qreal length, qreal halfWidth)
{
    QPointF dir = tip - tail;
    const qreal len = std::hypot(dir.x(), dir.y());
    if (len < 1e-6)
        return;
    dir /= len; // unit vector from tail to tip
    const QPointF normal(-dir.y(), dir.x());

    if (limiterStyle) {
        // A limiter is just a short bar perpendicular to the line at the tip.
        const QPointF a = tip + normal * halfWidth;
        const QPointF b = tip - normal * halfWidth;
        painter->drawLine(a, b);
        return;
    }

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
}

} // namespace PrimitiveArrowUtils

#endif // PRIMITIVEARROWUTILS_H
