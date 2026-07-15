/*
 * BooleanOps.cpp
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

#include "BooleanOps.h"
#include "GraphicsPrimitive.h"
#include "PrimitivePolygon.h"
#include "PrimitiveComplexCurve.h"
#include <QLineF>
#include <QPainterPath>
#include <QPolygonF>
#include <QVector>

namespace {

// One closed contour of the boolean result, kept as its own QPainterPath so
// it can be tested for containment (hole detection) and flattened/sampled
// independently of its siblings.
struct Contour {
    QPainterPath path;
    bool hasCurves = false;
    // Number of other contours this one lies inside: even = an outline of
    // its own, odd = a hole in the innermost contour containing it.
    int depth = 0;
};

// Distance under which two consecutive sampled/extracted points count as the
// same vertex - filters the duplicates that flattening and element-walking
// naturally produce at subpath seams.
constexpr qreal CoincidentDistance = 0.01;

// Sampling step (drawing units) along a contour when building a complex
// curve result. Also used to subdivide its straight runs: the FCD complex
// curve interpolates *all* its points with one smooth spline, so straight
// stretches only stay visually straight (and corners only stay visually
// sharp) if the sampled points sit this densely.
constexpr qreal SmoothSampleStep = 5.0;

void appendUnique(QVector<QPointF> &points, const QPointF &point)
{
    if (!points.isEmpty() && (points.last() - point).manhattanLength() < CoincidentDistance)
        return;
    points.append(point);
}

QPointF cubicPointAt(const QPointF &p0, const QPointF &c1, const QPointF &c2,
                     const QPointF &p1, qreal t)
{
    const qreal u = 1.0 - t;
    return u * u * u * p0 + 3.0 * u * u * t * c1 + 3.0 * u * t * t * c2 + t * t * t * p1;
}

qreal distance(const QPointF &a, const QPointF &b)
{
    return QLineF(a, b).length();
}

// Splits `path` into its closed contours, walking the raw elements so that
// "does this contour contain any real curve segment" is known exactly - the
// deciding factor between an exact polygon and a flattened/sampled result.
QList<Contour> splitContours(const QPainterPath &path)
{
    QList<Contour> contours;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element element = path.elementAt(i);
        switch (element.type) {
        case QPainterPath::MoveToElement:
            contours.append(Contour());
            contours.last().path.moveTo(element);
            break;
        case QPainterPath::LineToElement:
            contours.last().path.lineTo(element);
            break;
        case QPainterPath::CurveToElement: {
            // A CurveToElement is always followed by exactly two
            // CurveToDataElements holding the second control point and the
            // end point (see QPainterPath::elementAt() docs).
            const QPointF control1 = element;
            const QPointF control2 = path.elementAt(i + 1);
            const QPointF end = path.elementAt(i + 2);
            i += 2;
            contours.last().path.cubicTo(control1, control2, end);
            contours.last().hasCurves = true;
            break;
        }
        default:
            break;
        }
    }
    return contours;
}

// The exact vertices of a contour known to contain only straight edges.
QVector<QPointF> straightContourVertices(const QPainterPath &contour)
{
    QVector<QPointF> points;
    for (int i = 0; i < contour.elementCount(); ++i)
        appendUnique(points, contour.elementAt(i));
    if (points.size() > 2
            && (points.last() - points.first()).manhattanLength() < CoincidentDistance)
        points.removeLast();
    return points;
}

// Points sampled along a (possibly curved) contour, dense enough that the
// interpolating spline drawn through them stays visually faithful to the
// original outline - see SmoothSampleStep.
QVector<QPointF> sampledContourVertices(const QPainterPath &contour)
{
    QVector<QPointF> points;
    QPointF current;
    for (int i = 0; i < contour.elementCount(); ++i) {
        const QPainterPath::Element element = contour.elementAt(i);
        switch (element.type) {
        case QPainterPath::MoveToElement:
            current = element;
            appendUnique(points, current);
            break;
        case QPainterPath::LineToElement: {
            const QPointF end = element;
            const int steps = qMax(1, qRound(distance(current, end) / SmoothSampleStep));
            for (int s = 1; s <= steps; ++s)
                appendUnique(points, current + (end - current) * (qreal(s) / steps));
            current = end;
            break;
        }
        case QPainterPath::CurveToElement: {
            const QPointF control1 = element;
            const QPointF control2 = contour.elementAt(i + 1);
            const QPointF end = contour.elementAt(i + 2);
            i += 2;
            // The control polygon's length is a cheap upper bound of the
            // curve's - good enough to pick a sampling density.
            const qreal lengthBound = distance(current, control1)
                    + distance(control1, control2) + distance(control2, end);
            const int steps = qBound(4, qRound(lengthBound / SmoothSampleStep), 200);
            for (int s = 1; s <= steps; ++s)
                appendUnique(points, cubicPointAt(current, control1, control2, end,
                                                  qreal(s) / steps));
            current = end;
            break;
        }
        default:
            break;
        }
    }
    if (points.size() > 2
            && (points.last() - points.first()).manhattanLength() < CoincidentDistance)
        points.removeLast();
    return points;
}

// Uniformly drops points to fit the primitives' vertex cap - only ever
// reachable with a pathologically detailed result.
QVector<QPointF> capVertexCount(QVector<QPointF> points, int maxVertices)
{
    if (points.size() <= maxVertices)
        return points;
    QVector<QPointF> reduced;
    reduced.reserve(maxVertices);
    const qreal step = qreal(points.size()) / maxVertices;
    for (int i = 0; i < maxVertices; ++i)
        reduced.append(points.at(int(i * step)));
    return reduced;
}

GraphicsPrimitive *makePolygon(const QVector<QPointF> &vertices, GraphicsPrimitive *base)
{
    if (vertices.size() < 3)
        return nullptr;
    auto *polygon = new PrimitivePolygon();
    for (const QPointF &vertex : capVertexCount(vertices, PrimitivePolygon::MaxVertices))
        polygon->appendVertex(vertex);
    polygon->setLayer(base->layer());
    polygon->setIsFilled(base->isFilled());
    polygon->penStyleIsChanged(base->lineStyle());
    return polygon;
}

GraphicsPrimitive *makeComplexCurve(const QVector<QPointF> &vertices, GraphicsPrimitive *base)
{
    if (vertices.size() < 3)
        return nullptr;
    auto *curve = new PrimitiveComplexCurve();
    for (const QPointF &vertex : capVertexCount(vertices, PrimitiveComplexCurve::MaxVertices))
        curve->appendVertex(vertex);
    curve->setClosed(true);
    curve->setLayer(base->layer());
    curve->setIsFilled(base->isFilled());
    curve->penStyleIsChanged(base->lineStyle());
    return curve;
}

}

QVector<QPointF> BooleanOps::exactVertices(const QPainterPath &path)
{
    return straightContourVertices(path);
}

QVector<QPointF> BooleanOps::sampledVertices(const QPainterPath &path)
{
    return sampledContourVertices(path);
}

QList<GraphicsPrimitive *> BooleanOps::combine(const QList<GraphicsPrimitive *> &operands,
                                               Operation operation, bool smoothResults)
{
    QList<GraphicsPrimitive *> results;
    if (operands.size() < 2)
        return results;

    QPainterPath combined = operands.first()->booleanOutline();
    for (int i = 1; i < operands.size(); ++i) {
        const QPainterPath other = operands.at(i)->booleanOutline();
        switch (operation) {
        case Operation::Union:
            combined = combined.united(other);
            break;
        case Operation::Subtraction:
            combined = combined.subtracted(other);
            break;
        case Operation::Intersection:
            combined = combined.intersected(other);
            break;
        }
    }
    return primitivesFromPath(combined, operands.first(), smoothResults);
}

QList<GraphicsPrimitive *> BooleanOps::primitivesFromPath(const QPainterPath &path,
                                                          GraphicsPrimitive *styleSource,
                                                          bool smoothResults)
{
    QList<GraphicsPrimitive *> results;
    // simplified() merges intersecting subpaths and normalizes the fill rule
    // to odd-even, which is what makes the containment-depth hole test below
    // meaningful.
    const QPainterPath combined = path.simplified();
    if (combined.isEmpty())
        return results;

    QList<Contour> contours = splitContours(combined);

    // A contour lying inside an odd number of others is a hole. Testing the
    // first element point is enough: simplified() output contours never
    // cross, so any point of one decides containment for all of it.
    for (int i = 0; i < contours.size(); ++i) {
        for (int j = 0; j < contours.size(); ++j) {
            if (i != j && contours.at(j).path.contains(contours.at(i).path.elementAt(0)))
                ++contours[i].depth;
        }
    }

    GraphicsPrimitive *base = styleSource;
    for (int i = 0; i < contours.size(); ++i) {
        const Contour &contour = contours.at(i);
        if (contour.depth % 2 != 0)
            continue; // a hole - emitted together with its outline below

        // Collect the holes directly inside this outline (depth exactly one
        // deeper - anything deeper belongs to a nested outline of its own).
        QPainterPath withHoles = contour.path;
        bool hasHoles = false;
        for (int j = 0; j < contours.size(); ++j) {
            if (contours.at(j).depth == contour.depth + 1
                    && contour.path.contains(contours.at(j).path.elementAt(0))) {
                withHoles.addPath(contours.at(j).path);
                hasHoles = true;
            }
        }

        GraphicsPrimitive *result = nullptr;
        if (hasHoles) {
            // No multi-contour primitive exists in the format: fall back to
            // one flattened "keyhole" polygon, with the holes connected to
            // the outline through the zero-width seams toFillPolygon()
            // inserts between subpaths.
            QVector<QPointF> keyholeVertices;
            for (const QPointF &point : withHoles.toFillPolygon())
                appendUnique(keyholeVertices, point);
            if (keyholeVertices.size() > 2
                    && (keyholeVertices.last() - keyholeVertices.first()).manhattanLength() < CoincidentDistance)
                keyholeVertices.removeLast();
            result = makePolygon(keyholeVertices, base);
        } else if (!contour.hasCurves) {
            result = makePolygon(straightContourVertices(contour.path), base);
        } else if (smoothResults) {
            result = makeComplexCurve(sampledContourVertices(contour.path), base);
        } else {
            const QList<QPolygonF> flattened = contour.path.toSubpathPolygons();
            if (!flattened.isEmpty()) {
                QPainterPath flatPath;
                flatPath.addPolygon(flattened.first());
                result = makePolygon(straightContourVertices(flatPath), base);
            }
        }
        if (result)
            results.append(result);
    }
    return results;
}
