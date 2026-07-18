/*
 * MainWindowEditActions.cpp
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

// MainWindow's Edit menu / selection actions: Mirror, Rotate, Delete, Cut/
// Copy/Paste/Duplicate, Select all, Convert macro to primitives, Create
// macro from selection, Align and Distribute, plus the shared right-click
// context menu that reuses the same QAction objects. See MainWindow.cpp's
// top-of-file comment for why this lives in its own translation unit
// instead of inside MainWindow.cpp itself.

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "BooleanOps.h"
#include "DialogArray.h"
#include "FidoCadReader.h"
#include "FidoCadWriter.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitivePolygon.h"
#include "MirrorPrimitiveCommand.h"
#include "RotatePrimitiveCommand.h"
#include "DeletePrimitiveCommand.h"
#include "CreatePrimitiveCommand.h"
#include "MovePrimitiveCommand.h"
#include "InsertNodeCommand.h"
#include "RemoveNodeCommand.h"
#include "LibraryManager.h"
#include "PrimitiveMacro.h"
#include "PrimitiveImage.h"
#include "PrimitiveText.h"
#include "PrimitiveLine.h"
#include "DialogCreateMacro.h"
#include "DialogHatch.h"
#include "DialogFind.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QBuffer>
#include <QPainter>
#include <QMenu>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineF>
#include <QSet>
#include <QtMath>
#include <QEventLoop>
#include <QMouseEvent>
#include <QGraphicsLineItem>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>

namespace {
// Squared distance from `p` to the segment [a, b] - squared (not the actual
// distance) since only relative comparisons are needed below, avoiding a
// sqrt per candidate segment.
qreal pointToSegmentDistanceSq(const QPointF &p, const QPointF &a, const QPointF &b)
{
    const QPointF ab = b - a;
    const qreal lengthSq = QPointF::dotProduct(ab, ab);
    qreal t = lengthSq > 0 ? QPointF::dotProduct(p - a, ab) / lengthSq : 0.0;
    t = qBound<qreal>(0.0, t, 1.0);
    const QPointF closest = a + ab * t;
    const QPointF diff = p - closest;
    return QPointF::dotProduct(diff, diff);
}

// The vertex index of `primitive` closest to `scenePos` - candidate for
// "remove node".
int nearestVertexIndex(GraphicsPrimitive *primitive, const QPointF &scenePos)
{
    int nearest = 0;
    qreal bestDistSq = std::numeric_limits<qreal>::max();
    for (int i = 0; i < primitive->controlPointCount(); ++i) {
        const QPointF diff = primitive->controlPoint(i) - scenePos;
        const qreal distSq = QPointF::dotProduct(diff, diff);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            nearest = i;
        }
    }
    return nearest;
}

// The index at which a new vertex nearest `scenePos` should be inserted -
// found via point-to-segment distance to every existing edge (wrapping for a
// closed polygon/curve), so the new vertex lands on whichever edge the click
// actually landed nearest to, rather than just between the two nearest
// vertices by index.
int nearestSegmentInsertIndex(GraphicsPrimitive *primitive, const QPointF &scenePos)
{
    const int count = primitive->controlPointCount();
    const int segments = primitive->isClosedShape() ? count : count - 1;
    int bestIndex = count;
    qreal bestDistSq = std::numeric_limits<qreal>::max();
    for (int i = 0; i < segments; ++i) {
        const QPointF a = primitive->controlPoint(i);
        const QPointF b = primitive->controlPoint((i + 1) % count);
        const qreal distSq = pointToSegmentDistanceSq(scenePos, a, b);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestIndex = i + 1;
        }
    }
    return bestIndex;
}

// One-shot canvas point picker behind the array dialog's "Pick from canvas":
// crosshair cursor, the same object/grid snapping as a drawing click (live
// highlight included), left click accepts, right click or Escape cancels.
// Runs a local event loop so the caller reads the result synchronously; the
// dialog that asked for the pick has already closed itself, so no modal
// window is left to block the canvas.
class ScenePointPicker : public QObject
{
public:
    // `snap` false picks the raw scene position instead of the object/grid
    // snapped one - used when the click selects *which part* of something
    // was hit (trim/extend) rather than an exact coordinate, where snapping
    // could jump the pick across the very intersection being targeted.
    ScenePointPicker(SheetView *view, Sheet *sheet, bool snap = true)
        : m_view(view), m_sheet(sheet), m_snap(snap) {}

    // Invoked with the raw scene position on every mouse move while the
    // pick is running - lets the caller drive an AutoCAD-style live preview
    // of what clicking right here would do (trim/extend overlays).
    void setHoverCallback(std::function<void(const QPointF &)> callback)
    {
        m_hoverCallback = std::move(callback);
    }

    // Primitives the snapped pick must ignore - callers whose live preview
    // is made of real primitives on the sheet (Move/Copy with base point)
    // pass them here, or the preview's own vertices would become snap and
    // tracking targets that chase the cursor around.
    void setExcluded(const QList<const GraphicsPrimitive *> &excluded)
    {
        m_excluded = excluded;
    }

    bool run(QPointF *picked)
    {
        m_view->viewport()->installEventFilter(this);
        m_view->installEventFilter(this); // Escape lands on the view itself
        m_view->viewport()->setCursor(Qt::CrossCursor);
        m_view->setFocus();
        m_loop.exec();
        m_view->viewport()->removeEventFilter(this);
        m_view->removeEventFilter(this);
        m_view->viewport()->unsetCursor();
        m_sheet->clearSnapIndicator();
        if (m_accepted)
            *picked = m_point;
        return m_accepted;
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_view->viewport()) {
            if (event->type() == QEvent::MouseButtonPress) {
                auto *mouse = static_cast<QMouseEvent *>(event);
                if (mouse->button() == Qt::LeftButton) {
                    const QPointF scenePos =
                            m_view->mapToScene(mouse->position().toPoint());
                    m_point = m_snap ? m_sheet->snapPosition(scenePos, m_excluded) : scenePos;
                    m_accepted = true;
                    m_loop.quit();
                    return true; // the click must not also select/draw anything
                }
                if (mouse->button() == Qt::RightButton) {
                    m_loop.quit();
                    return true;
                }
            } else if (event->type() == QEvent::MouseMove) {
                // Live object-snap highlight while hunting for the point.
                // The move must be swallowed: SheetView's own mouseMoveEvent
                // would otherwise clear the highlight right back (its
                // "not placing anything" branch) and reset the cursor.
                const QPointF scenePos = m_view->mapToScene(
                        static_cast<QMouseEvent *>(event)->position().toPoint());
                if (m_snap)
                    m_sheet->snapPosition(scenePos, m_excluded);
                if (m_hoverCallback)
                    m_hoverCallback(scenePos);
                return true;
            }
        }
        if (event->type() == QEvent::KeyPress
                && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            m_loop.quit();
            return true;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    SheetView *m_view;
    Sheet *m_sheet;
    bool m_snap;
    QList<const GraphicsPrimitive *> m_excluded;
    std::function<void(const QPointF &)> m_hoverCallback;
    QEventLoop m_loop;
    QPointF m_point;
    bool m_accepted = false;
};

// --- Shape rewriting helpers (Convert/Simplify/Fillet/Chamfer) ------------

bool convertibleToPolygon(GraphicsPrimitive *primitive)
{
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Rectangle:
    case GraphicsPrimitive::Ellipse:
        return true;
    case GraphicsPrimitive::Spline:
        return primitive->isClosedShape();
    default:
        return false;
    }
}

bool convertibleToCurve(GraphicsPrimitive *primitive)
{
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Rectangle:
    case GraphicsPrimitive::Ellipse:
        return true;
    case GraphicsPrimitive::Polyline:
        return primitive->controlPointCount() >= 3;
    default:
        return false;
    }
}

// The vertex list a conversion should produce for `primitive`:
// - rectangle: its 4 exact corners (both targets);
// - ellipse -> curve: a sparse parametric sample - a spline through 16
//   points tracks an ellipse closely while staying comfortably editable;
// - ellipse/closed curve -> polygon: densely sampled outline;
// - polygon -> curve: the very same vertices, reinterpreted as spline nodes
//   (that's the point: a "smoothed" version of the polygon).
QVector<QPointF> conversionVertices(GraphicsPrimitive *primitive, bool toCurve)
{
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Rectangle:
        return BooleanOps::exactVertices(primitive->booleanOutline());
    case GraphicsPrimitive::Ellipse: {
        if (toCurve) {
            const QRectF rect = QRectF(primitive->controlPoint(0),
                                       primitive->controlPoint(1)).normalized();
            QVector<QPointF> points;
            constexpr int EllipseCurveNodes = 16;
            points.reserve(EllipseCurveNodes);
            for (int i = 0; i < EllipseCurveNodes; ++i) {
                const qreal angle = 2.0 * M_PI * i / EllipseCurveNodes;
                points.append(rect.center() + QPointF(rect.width() / 2.0 * std::cos(angle),
                                                      rect.height() / 2.0 * std::sin(angle)));
            }
            return points;
        }
        return BooleanOps::sampledVertices(primitive->booleanOutline());
    }
    case GraphicsPrimitive::Polyline: {
        QVector<QPointF> points;
        points.reserve(primitive->controlPointCount());
        for (int i = 0; i < primitive->controlPointCount(); ++i)
            points.append(primitive->controlPoint(i));
        return points;
    }
    case GraphicsPrimitive::Spline:
        return BooleanOps::sampledVertices(primitive->booleanOutline());
    default:
        return {};
    }
}

// A fresh polygon (asCurve false) or closed complex curve (asCurve true)
// through `vertices`, inheriting `source`'s layer, fill, dash style and
// name/value label - the shared "rewrite this shape" constructor behind
// Convert/Simplify/Fillet/Chamfer.
GraphicsPrimitive *makeShapeLike(GraphicsPrimitive *source, const QVector<QPointF> &vertices,
                                 bool asCurve, bool closed = true)
{
    if (vertices.size() < (closed ? 3 : 2))
        return nullptr;
    GraphicsPrimitive *shape = nullptr;
    if (asCurve) {
        auto *curve = new PrimitiveComplexCurve();
        for (const QPointF &vertex : vertices)
            curve->appendVertex(vertex);
        curve->setClosed(closed);
        shape = curve;
    } else {
        auto *polygon = new PrimitivePolygon();
        for (const QPointF &vertex : vertices)
            polygon->appendVertex(vertex);
        shape = polygon;
    }
    shape->setLayer(source->layer());
    shape->setIsFilled(source->isFilled());
    shape->penStyleIsChanged(source->lineStyle());
    shape->setName(source->name());
    shape->setValue(source->value());
    return shape;
}

// --- Split / Trim / Extend helpers -----------------------------------------

// A fresh, independent copy of one primitive via the same FCD serialize/
// parse round trip Duplicate relies on - style, layer, arrows and labels
// all come along.
GraphicsPrimitive *clonePrimitive(GraphicsPrimitive *primitive, Sheet *sheet)
{
    QList<GraphicsPrimitive *> clones = FidoCadReader::parse(
            FidoCadWriter::writeSelection({ primitive }), sheet);
    if (clones.size() == 1)
        return clones.first();
    qDeleteAll(clones);
    return nullptr;
}

// The parameter (clamped to [0, 1]) of `p`'s projection onto segment [a, b].
qreal segmentParam(const QPointF &p, const QPointF &a, const QPointF &b)
{
    const QPointF ab = b - a;
    const qreal lengthSq = QPointF::dotProduct(ab, ab);
    if (lengthSq <= 0.0)
        return 0.0;
    return qBound<qreal>(0.0, QPointF::dotProduct(p - a, ab) / lengthSq, 1.0);
}

// What Split at point accepts: the open, line-like primitives. Closed
// shapes have no "two halves" to split into and are served by the boolean
// operations instead.
bool splittableAtPoint(GraphicsPrimitive *primitive)
{
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Line:
    case GraphicsPrimitive::PcbTrack:
    case GraphicsPrimitive::Bezier:
        return true;
    case GraphicsPrimitive::Spline:
        return !primitive->isClosedShape() && primitive->controlPointCount() >= 2;
    default:
        return false;
    }
}

void copyArrowAttributes(GraphicsPrimitive *destination, const GraphicsPrimitive *source)
{
    destination->setArrowAtStart(source->arrowAtStart());
    destination->setArrowAtEnd(source->arrowAtEnd());
    destination->setArrowStyleLimiter(source->arrowStyleLimiter());
    destination->setArrowStyleEmpty(source->arrowStyleEmpty());
    destination->setArrowLength(source->arrowLength());
    destination->setArrowHalfWidth(source->arrowHalfWidth());
}

// The outline actually drawn for `primitive`, flattened to polylines in
// scene coordinates - the cutting/boundary edges trim and extend test
// against. Empty for primitive types that make no sense as a boundary
// (text, images, connection dots, pads, macros).
QList<QPolygonF> boundaryPolylines(GraphicsPrimitive *primitive)
{
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Line:
    case GraphicsPrimitive::PcbTrack: {
        QPolygonF chain;
        chain << primitive->controlPoint(0) << primitive->controlPoint(1);
        return { chain };
    }
    case GraphicsPrimitive::Rectangle:
    case GraphicsPrimitive::Ellipse:
    case GraphicsPrimitive::Polyline:
        return primitive->booleanOutline().toSubpathPolygons();
    case GraphicsPrimitive::Spline:
        return static_cast<PrimitiveComplexCurve *>(primitive)
                ->curvePath().toSubpathPolygons();
    case GraphicsPrimitive::Bezier: {
        QPainterPath path;
        path.moveTo(primitive->controlPoint(0));
        path.cubicTo(primitive->controlPoint(1), primitive->controlPoint(2),
                     primitive->controlPoint(3));
        return path.toSubpathPolygons();
    }
    default:
        return {};
    }
}

// --- Offset helpers ---------------------------------------------------------

// What Edit > Shape > Offset accepts: every base drawing primitive except
// macros, images, connection dots, texts, pads and PCB tracks.
bool offsettable(GraphicsPrimitive *primitive)
{
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Line:
    case GraphicsPrimitive::Bezier:
    case GraphicsPrimitive::Rectangle:
    case GraphicsPrimitive::Ellipse:
    case GraphicsPrimitive::Polyline:
        return true;
    case GraphicsPrimitive::Spline:
        return primitive->controlPointCount() >= 2;
    default:
        return false;
    }
}

// Miter-parallel offset of a point chain: exactly one output vertex per
// input vertex - each is the intersection of its two adjacent edges shifted
// along their normals - so polygons and curves keep their node count
// instead of gaining spurious vertices from a stroked-path union. The
// distance's sign picks the side; near-degenerate corners fall back to a
// plain shift so no miter spike can explode.
QVector<QPointF> offsetChain(const QVector<QPointF> &points, qreal distance, bool closed)
{
    const int count = points.size();
    if (count < 2 || qFuzzyIsNull(distance))
        return points;

    auto edgeNormal = [](const QPointF &a, const QPointF &b) {
        const QPointF direction = b - a;
        const qreal length = std::hypot(direction.x(), direction.y());
        if (length <= 0)
            return QPointF();
        return QPointF(direction.y() / length, -direction.x() / length);
    };

    QVector<QPointF> result(count);
    for (int i = 0; i < count; ++i) {
        const bool hasPrev = closed || i > 0;
        const bool hasNext = closed || i < count - 1;
        const QPointF previous = points.at((i - 1 + count) % count);
        const QPointF current = points.at(i);
        const QPointF next = points.at((i + 1) % count);
        const QPointF nPrev = hasPrev ? edgeNormal(previous, current) : QPointF();
        const QPointF nNext = hasNext ? edgeNormal(current, next) : QPointF();

        if (hasPrev && hasNext && !nPrev.isNull() && !nNext.isNull()) {
            const QLineF shiftedPrev(previous + nPrev * distance, current + nPrev * distance);
            const QLineF shiftedNext(current + nNext * distance, next + nNext * distance);
            QPointF corner;
            if (shiftedPrev.intersects(shiftedNext, &corner) == QLineF::NoIntersection) {
                corner = current + nPrev * distance; // collinear edges
            } else {
                const QPointF spike = corner - current;
                if (std::hypot(spike.x(), spike.y()) > qAbs(distance) * 10.0)
                    corner = current + nPrev * distance; // clamp absurd miters
            }
            result[i] = corner;
        } else if (hasNext && !nNext.isNull()) {
            result[i] = current + nNext * distance;
        } else if (hasPrev && !nPrev.isNull()) {
            result[i] = current + nPrev * distance;
        } else {
            result[i] = current;
        }
    }
    return result;
}

// Distance from `p` to the chain's nearest segment - how the cursor picks
// which of the two offset sides it means.
qreal chainDistanceTo(const QVector<QPointF> &points, bool closed, const QPointF &p)
{
    const int count = points.size();
    if (count == 0)
        return std::numeric_limits<qreal>::max();
    if (count == 1) {
        const QPointF diff = p - points.first();
        return std::hypot(diff.x(), diff.y());
    }
    qreal best = std::numeric_limits<qreal>::max();
    const int segments = closed ? count : count - 1;
    for (int i = 0; i < segments; ++i)
        best = qMin(best, pointToSegmentDistanceSq(p, points.at(i), points.at((i + 1) % count)));
    return std::sqrt(best);
}

// Twice the signed area of a closed chain - its sign encodes the winding,
// so a flipped sign after an inward offset means the shape was eroded away.
qreal signedAreaTwice(const QVector<QPointF> &points)
{
    qreal area = 0;
    const int count = points.size();
    for (int i = 0; i < count; ++i) {
        const QPointF &a = points.at(i);
        const QPointF &b = points.at((i + 1) % count);
        area += a.x() * b.y() - b.x() * a.y();
    }
    return area;
}

qreal rectOutlineDistanceTo(const QRectF &rect, const QPointF &p)
{
    QVector<QPointF> corners = { rect.topLeft(), rect.topRight(),
                                 rect.bottomRight(), rect.bottomLeft() };
    return chainDistanceTo(corners, true, p);
}

// --- Ramer-Douglas-Peucker simplification ---------------------------------

void rdpMarkKept(const QVector<QPointF> &points, int first, int last,
                 qreal toleranceSq, QVector<bool> &keep)
{
    qreal maxDistSq = -1.0;
    int farthest = -1;
    for (int i = first + 1; i < last; ++i) {
        const qreal distSq = pointToSegmentDistanceSq(points.at(i), points.at(first), points.at(last));
        if (distSq > maxDistSq) {
            maxDistSq = distSq;
            farthest = i;
        }
    }
    if (farthest >= 0 && maxDistSq > toleranceSq) {
        keep[farthest] = true;
        rdpMarkKept(points, first, farthest, toleranceSq, keep);
        rdpMarkKept(points, farthest, last, toleranceSq, keep);
    }
}

QVector<QPointF> simplifiedChain(const QVector<QPointF> &points, qreal tolerance, bool closed)
{
    if (points.size() < 3)
        return points;
    QVector<bool> keep(points.size(), false);
    keep.first() = true;
    keep.last() = true;
    const qreal toleranceSq = tolerance * tolerance;
    if (closed) {
        // Anchor a third point - the one farthest from vertex 0 - so a
        // closed ring can't collapse onto its own start vertex. The wrap
        // edge (last back to first) is left as drawn.
        int farthest = 0;
        qreal maxDistSq = -1.0;
        for (int i = 1; i < points.size(); ++i) {
            const QPointF diff = points.at(i) - points.first();
            const qreal distSq = QPointF::dotProduct(diff, diff);
            if (distSq > maxDistSq) {
                maxDistSq = distSq;
                farthest = i;
            }
        }
        keep[farthest] = true;
        rdpMarkKept(points, 0, farthest, toleranceSq, keep);
        rdpMarkKept(points, farthest, points.size() - 1, toleranceSq, keep);
    } else {
        rdpMarkKept(points, 0, points.size() - 1, toleranceSq, keep);
    }
    QVector<QPointF> result;
    for (int i = 0; i < points.size(); ++i) {
        if (keep.at(i))
            result.append(points.at(i));
    }
    return result;
}

// --- Corner rounding -------------------------------------------------------

// Rewrites every corner of the closed ring `points`: chamfer replaces the
// vertex with a straight cut at `amount` along each edge; fillet replaces it
// with a sampled circular arc of radius `amount` tangent to both edges. The
// cut/tangent distance is clamped to just under half of each adjacent edge,
// so neighboring corners never overlap.
QVector<QPointF> roundedCornerRing(const QVector<QPointF> &points, qreal amount, bool chamfer)
{
    const int n = points.size();
    QVector<QPointF> result;
    for (int i = 0; i < n; ++i) {
        const QPointF previous = points.at((i - 1 + n) % n);
        const QPointF vertex = points.at(i);
        const QPointF next = points.at((i + 1) % n);
        const QPointF edge1 = previous - vertex;
        const QPointF edge2 = next - vertex;
        const qreal length1 = std::hypot(edge1.x(), edge1.y());
        const qreal length2 = std::hypot(edge2.x(), edge2.y());
        if (length1 < 1e-6 || length2 < 1e-6) {
            result.append(vertex);
            continue;
        }
        const QPointF unit1 = edge1 / length1;
        const QPointF unit2 = edge2 / length2;
        const qreal interior = std::acos(qBound(-1.0, QPointF::dotProduct(unit1, unit2), 1.0));
        if (interior < 1e-3 || interior > M_PI - 1e-3) {
            result.append(vertex); // straight or degenerate corner - nothing to round
            continue;
        }
        qreal distance = chamfer ? amount : amount / std::tan(interior / 2.0);
        distance = qMin(distance, qMin(length1, length2) / 2.0 - 0.01);
        if (distance <= 0.01) {
            result.append(vertex);
            continue;
        }
        const QPointF tangent1 = vertex + unit1 * distance;
        const QPointF tangent2 = vertex + unit2 * distance;
        if (chamfer) {
            result.append(tangent1);
            result.append(tangent2);
            continue;
        }
        // Fillet: the arc's center sits on the corner's bisector, at the
        // distance that makes the circle tangent to both edges at
        // tangent1/tangent2.
        QPointF bisector = unit1 + unit2;
        const qreal bisectorLength = std::hypot(bisector.x(), bisector.y());
        if (bisectorLength < 1e-6) {
            result.append(vertex);
            continue;
        }
        bisector /= bisectorLength;
        const QPointF center = vertex + bisector * (distance / std::cos(interior / 2.0));
        const qreal radius = QLineF(center, tangent1).length();
        const qreal angle1 = std::atan2(tangent1.y() - center.y(), tangent1.x() - center.x());
        const qreal angle2 = std::atan2(tangent2.y() - center.y(), tangent2.x() - center.x());
        qreal sweep = angle2 - angle1;
        while (sweep > M_PI)
            sweep -= 2.0 * M_PI;
        while (sweep <= -M_PI)
            sweep += 2.0 * M_PI;
        const int steps = qBound(2, qRound(radius * qAbs(sweep) / 3.0), 32);
        for (int k = 0; k <= steps; ++k) {
            const qreal angle = angle1 + sweep * k / steps;
            result.append(center + radius * QPointF(std::cos(angle), std::sin(angle)));
        }
    }
    return result;
}
}

// Mirror/Rotate/Delete/SelectAll all operate on the current scene selection.
// Mirror and Rotate pivot around controlPoint(0) of the first selected
// primitive in document order - matching the reference FidoCadJ editor
// (EditorActions.rotateAllSelected/mirrorAllSelected pivot on
// getFirstSelectedPrimitive().getFirstPoint()), rather than e.g. the
// selection's bounding-box center.
void MainWindow::scheduleSelectionRefresh()
{
    if (m_selectionRefreshPending)
        return;
    m_selectionRefreshPending = true;
    QMetaObject::invokeMethod(this, [this]() {
        m_selectionRefreshPending = false;
        updateEditActionsState();
        updatePropertiesPanel();
    }, Qt::QueuedConnection);
}

void MainWindow::updateEditActionsState()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    const bool hasSelection = !selected.isEmpty();

    bool hasMacro = false;
    for (GraphicsPrimitive *primitive : selected) {
        if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib) {
            hasMacro = true;
            break;
        }
    }

    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionCopySplit->setEnabled(hasSelection);
    ui->actionDuplicate->setEnabled(hasSelection);
    ui->actionDelete->setEnabled(hasSelection);
    ui->actionMirror->setEnabled(hasSelection);
    ui->actionMirrorCopy->setEnabled(hasSelection);
    ui->actionRotate->setEnabled(hasSelection);
    ui->actionCreateMacro->setEnabled(hasSelection);
    ui->actionConvertMacroToPrimitives->setEnabled(hasMacro);

    // Cached on QClipboard::dataChanged (see setConnections()) - never read
    // the system clipboard here, this runs on every selection change.
    ui->actionPaste->setEnabled(m_clipboardPastable);
    ui->actionPasteInPlace->setEnabled(m_clipboardPastable);

    const bool hasAtLeastTwo = selected.size() >= 2;
    ui->actionAlignLeft->setEnabled(hasAtLeastTwo);
    ui->actionAlignRight->setEnabled(hasAtLeastTwo);
    ui->actionAlignTop->setEnabled(hasAtLeastTwo);
    ui->actionAlignBottom->setEnabled(hasAtLeastTwo);
    ui->actionAlignCenterHorizontal->setEnabled(hasAtLeastTwo);
    ui->actionAlignCenterVertical->setEnabled(hasAtLeastTwo);

    const bool hasAtLeastThree = selected.size() >= 3;
    ui->actionDistributeHorizontal->setEnabled(hasAtLeastThree);
    ui->actionDistributeVertical->setEnabled(hasAtLeastThree);

    // Boolean operations need at least two closed shapes among the selection
    // - only rectangles, ellipses, polygons and closed complex curves
    // qualify; macros, images, pads and the open primitives never do.
    int booleanOperands = 0;
    for (GraphicsPrimitive *primitive : selected) {
        if (primitive->supportsBooleanOps())
            ++booleanOperands;
    }
    const bool canCombine = booleanOperands >= 2;
    ui->actionBooleanUnion->setEnabled(canCombine);
    ui->actionBooleanSubtract->setEnabled(canCombine);
    ui->actionBooleanIntersect->setEnabled(canCombine);
    // Hatch fills any single closed shape - one qualifying operand is enough.
    ui->actionHatch->setEnabled(booleanOperands >= 1);

    bool canToPolygon = false, canToCurve = false, canSimplify = false, canRound = false;
    for (GraphicsPrimitive *primitive : selected) {
        canToPolygon = canToPolygon || convertibleToPolygon(primitive);
        canToCurve = canToCurve || convertibleToCurve(primitive);
        const GraphicsPrimitive::PrimitiveTypes type = primitive->getPrimitiveType();
        canSimplify = canSimplify
                || ((type == GraphicsPrimitive::Polyline || type == GraphicsPrimitive::Spline)
                    && primitive->controlPointCount() > 3);
        canRound = canRound || type == GraphicsPrimitive::Rectangle
                || (type == GraphicsPrimitive::Polyline && primitive->controlPointCount() >= 3);
    }
    ui->actionConvertToPolygon->setEnabled(canToPolygon);
    ui->actionConvertToCurve->setEnabled(canToCurve);
    ui->actionSimplifyNodes->setEnabled(canSimplify);
    ui->actionFilletCorners->setEnabled(canRound);
    ui->actionChamferCorners->setEnabled(canRound);
    ui->actionOffsetOutline->setEnabled(selected.size() == 1
                                        && offsettable(selected.first()));
    ui->actionSplitAtPoint->setEnabled(selected.size() == 1
                                       && splittableAtPoint(selected.first()));

    ui->actionScaleSelection->setEnabled(hasSelection);
    ui->actionRotateByAngle->setEnabled(hasSelection);
    ui->actionMoveBasePoint->setEnabled(hasSelection);
    ui->actionCopyBasePoint->setEnabled(hasSelection);
    ui->actionArray->setEnabled(hasSelection);
    ui->actionSnapSelectionToGrid->setEnabled(hasSelection);
    ui->actionSelectSameType->setEnabled(hasSelection);
    ui->actionZoomToSelection->setEnabled(hasSelection);
}

// Reuses the very same ui->action* objects the Edit menu bar entry is
// built from (rather than a second, parallel set wired to the same slots),
// so this menu's content, enabled state, shortcuts and icons can never drift
// out of sync with the menu bar - see updateEditActionsState().
void MainWindow::showCanvasContextMenu(const QPoint &globalPos, const QPointF &scenePos)
{
    QMenu menu(this);
    menu.addAction(ui->actionUndo);
    menu.addAction(ui->actionRestore);
    menu.addSeparator();
    menu.addAction(ui->actionCut);
    menu.addAction(ui->actionCopy);
    menu.addAction(ui->actionPaste);
    menu.addAction(ui->actionPasteInPlace);
    menu.addAction(ui->actionDuplicate);
    menu.addAction(ui->actionDelete);
    menu.addSeparator();
    menu.addAction(ui->actionMoveBasePoint);
    menu.addAction(ui->actionCopyBasePoint);
    menu.addAction(ui->actionRotate);
    menu.addAction(ui->actionMirror);
    menu.addAction(ui->actionMirrorCopy);
    menu.addAction(ui->actionRotateByAngle);
    menu.addAction(ui->actionScaleSelection);
    menu.addAction(ui->actionArray);
    menu.addSeparator();
    menu.addAction(ui->actionBooleanUnion);
    menu.addAction(ui->actionBooleanSubtract);
    menu.addAction(ui->actionBooleanIntersect);
    menu.addMenu(ui->menuShape);
    menu.addSeparator();
    menu.addAction(ui->actionConvertMacroToPrimitives);
    menu.addAction(ui->actionCreateMacro);
    menu.addSeparator();
    menu.addMenu(ui->menuAlign);
    menu.addSeparator();
    menu.addAction(ui->actionSelectAll);
    menu.addAction(ui->actionInvertSelection);
    menu.addAction(ui->actionSelectSameType);

    // Only meaningful for a single selected polygon/complex curve - added
    // here as plain transient QActions (the menu is rebuilt fresh every
    // time) rather than as ui-> actions, since whether they even appear
    // depends on what's selected.
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() == 1 && selected.first()->supportsNodeEditing()) {
        GraphicsPrimitive *primitive = selected.first();
        menu.addSeparator();
        QAction *addNodeAction = menu.addAction(tr("Add node"));
        connect(addNodeAction, &QAction::triggered, this, [this, primitive, scenePos]() {
            addNodeToPrimitive(primitive, scenePos);
        });
        QAction *removeNodeAction = menu.addAction(tr("Remove node"));
        removeNodeAction->setEnabled(primitive->controlPointCount() > 2);
        connect(removeNodeAction, &QAction::triggered, this, [this, primitive, scenePos]() {
            removeNodeFromPrimitive(primitive, scenePos);
        });
    }

    menu.exec(globalPos);
}

// Mirror/Rotate/Delete are never applied directly here - each pushes an undo
// command instead, whose redo() (auto-invoked once by QUndoStack::push()) is
// the sole place the actual mutation happens. Multiple selected primitives
// are wrapped in a macro so they undo/redo together as one step.
void MainWindow::clickMirrorAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Mirror"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new MirrorPrimitiveCommand(primitive, Qt::Horizontal, pivot));
    }
    if (multiple)
        undo->endMacro();
}

// Mirror as copy: the mirrored result is a fresh clone of the selection
// (same FCD serialize/parse round trip as Duplicate), so the original stays
// untouched in place - the classic "mirror copy" of CAD packages. The pivot
// matches clickMirrorAction()'s convention, so the copy lands exactly where
// a plain Mirror would have moved the original.
void MainWindow::clickMirrorCopyAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    const QPointF pivot = selected.first()->controlPoint(0);

    const QList<GraphicsPrimitive *> clones =
            FidoCadReader::parse(FidoCadWriter::writeSelection(selected), sheetScene);
    if (clones.isEmpty())
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Mirror as copy"));
    for (GraphicsPrimitive *clone : clones) {
        clone->mirror(Qt::Horizontal, pivot);
        // push() calls redo() synchronously, which is what actually adds the
        // primitive to the scene - only safe to select it afterwards.
        undo->push(new CreatePrimitiveCommand(sheetScene, clone));
        clone->setSelected(true);
    }
    undo->endMacro();
}

void MainWindow::clickRotateAction()
{
    GraphicsPrimitive *pivotPrimitive = firstSelectedPrimitive();
    if (!pivotPrimitive)
        return;
    const QPointF pivot = pivotPrimitive->controlPoint(0);

    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Rotate"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new RotatePrimitiveCommand(primitive, pivot));
    }
    if (multiple)
        undo->endMacro();
}

// Replaces the eligible selected shapes with the result of the boolean
// operation, deleting the operands and creating the result primitives inside
// one undo macro - same delete+create pattern as Convert macro to primitives.
// The first operand in document order is the base (what the others are
// subtracted from) and lends the result its layer, fill and dash style.
void MainWindow::applyBooleanOperation(BooleanOps::Operation operation, const QString &undoLabel)
{
    QList<GraphicsPrimitive *> operands;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        if (primitive->supportsBooleanOps())
            operands.append(primitive);
    }
    if (operands.size() < 2)
        return;

    const bool smooth = ui->actionBooleanSmooth->isChecked();
    const QList<GraphicsPrimitive *> results = BooleanOps::combine(operands, operation, smooth);
    // An empty region (e.g. intersecting disjoint shapes) produces nothing:
    // leave the drawing and the selection as they are rather than silently
    // deleting the operands.
    if (results.isEmpty())
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *operand : operands)
        undo->push(new DeletePrimitiveCommand(sheetScene, operand));
    for (GraphicsPrimitive *result : results) {
        // push() calls redo() synchronously, which is what actually adds the
        // primitive to the scene - only safe to select it afterwards.
        undo->push(new CreatePrimitiveCommand(sheetScene, result));
        result->setSelected(true);
    }
    undo->endMacro();
}

void MainWindow::clickBooleanUnionAction()
{
    applyBooleanOperation(BooleanOps::Operation::Union, tr("Union"));
}

void MainWindow::clickBooleanSubtractAction()
{
    applyBooleanOperation(BooleanOps::Operation::Subtraction, tr("Subtraction"));
}

void MainWindow::clickBooleanIntersectAction()
{
    applyBooleanOperation(BooleanOps::Operation::Intersection, tr("Intersection"));
}

void MainWindow::convertSelectionTo(bool toCurve, const QString &undoLabel)
{
    QList<GraphicsPrimitive *> eligible;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        if (toCurve ? convertibleToCurve(primitive) : convertibleToPolygon(primitive))
            eligible.append(primitive);
    }
    if (eligible.isEmpty())
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : eligible) {
        GraphicsPrimitive *converted =
                makeShapeLike(primitive, conversionVertices(primitive, toCurve), toCurve);
        if (!converted)
            continue;
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
        // push() calls redo() synchronously, which is what actually adds the
        // primitive to the scene - only safe to select it afterwards.
        undo->push(new CreatePrimitiveCommand(sheetScene, converted));
        converted->setSelected(true);
    }
    undo->endMacro();
}

void MainWindow::clickConvertToPolygonAction()
{
    convertSelectionTo(false, tr("Convert to polygon"));
}

void MainWindow::clickConvertToCurveAction()
{
    convertSelectionTo(true, tr("Convert to complex curve"));
}

void MainWindow::clickSimplifyNodesAction()
{
    QList<GraphicsPrimitive *> eligible;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        const GraphicsPrimitive::PrimitiveTypes type = primitive->getPrimitiveType();
        if ((type == GraphicsPrimitive::Polyline || type == GraphicsPrimitive::Spline)
                && primitive->controlPointCount() > 3)
            eligible.append(primitive);
    }
    if (eligible.isEmpty())
        return;

    bool ok = false;
    const double tolerance = QInputDialog::getDouble(
                this, tr("Simplify nodes"), tr("Tolerance (drawing units):"),
                1.0, 0.05, 100.0, 2, &ok);
    if (!ok)
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Simplify nodes"));
    for (GraphicsPrimitive *primitive : eligible) {
        QVector<QPointF> vertices;
        vertices.reserve(primitive->controlPointCount());
        for (int i = 0; i < primitive->controlPointCount(); ++i)
            vertices.append(primitive->controlPoint(i));

        const bool closed = primitive->isClosedShape();
        const QVector<QPointF> simplified = simplifiedChain(vertices, tolerance, closed);
        if (simplified.size() >= vertices.size()) {
            primitive->setSelected(true); // nothing removed - keep it as is
            continue;
        }

        const bool asCurve = primitive->getPrimitiveType() == GraphicsPrimitive::Spline;
        GraphicsPrimitive *replacement = makeShapeLike(primitive, simplified, asCurve, closed);
        if (!replacement) {
            primitive->setSelected(true);
            continue;
        }
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
        undo->push(new CreatePrimitiveCommand(sheetScene, replacement));
        replacement->setSelected(true);
    }
    undo->endMacro();
}

void MainWindow::roundSelectedCorners(bool chamfer)
{
    QList<GraphicsPrimitive *> eligible;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        const GraphicsPrimitive::PrimitiveTypes type = primitive->getPrimitiveType();
        if (type == GraphicsPrimitive::Rectangle
                || (type == GraphicsPrimitive::Polyline && primitive->controlPointCount() >= 3))
            eligible.append(primitive);
    }
    if (eligible.isEmpty())
        return;

    bool ok = false;
    const double amount = QInputDialog::getDouble(
                this,
                chamfer ? tr("Chamfer corners") : tr("Fillet corners"),
                chamfer ? tr("Cut distance (drawing units):") : tr("Radius (drawing units):"),
                5.0, 0.1, 1000.0, 1, &ok);
    if (!ok)
        return;

    const QString undoLabel = chamfer ? tr("Chamfer corners") : tr("Fillet corners");
    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : eligible) {
        // A rectangle contributes its 4 exact corners; the result is always
        // a polygon (fillet arcs are sampled into it).
        const QVector<QPointF> ring = primitive->getPrimitiveType() == GraphicsPrimitive::Rectangle
                ? BooleanOps::exactVertices(primitive->booleanOutline())
                : conversionVertices(primitive, false);
        GraphicsPrimitive *rounded =
                makeShapeLike(primitive, roundedCornerRing(ring, amount, chamfer), false);
        if (!rounded)
            continue;
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
        undo->push(new CreatePrimitiveCommand(sheetScene, rounded));
        rounded->setSelected(true);
    }
    undo->endMacro();
}

void MainWindow::clickFilletCornersAction()
{
    roundSelectedCorners(false);
}

QList<GraphicsPrimitive *> MainWindow::buildOffsetPrimitives(GraphicsPrimitive *source,
                                                             qreal distance,
                                                             const QPointF &cursor) const
{
    const GraphicsPrimitive::PrimitiveTypes type = source->getPrimitiveType();

    // Rectangles and ellipses stay their own type: the defining rect grown
    // or shrunk by the distance on each side. For a rectangle that IS the
    // exact parallel offset; for an ellipse it's the natural CAD result
    // (offset semi-axes - the mathematically exact offset curve isn't an
    // ellipse at all). The cursor picks the side by whichever candidate
    // outline it is closer to.
    if (type == GraphicsPrimitive::Rectangle || type == GraphicsPrimitive::Ellipse) {
        const QRectF rect = QRectF(source->controlPoint(0),
                                   source->controlPoint(1)).normalized();
        const QRectF grown = rect.adjusted(-distance, -distance, distance, distance);
        const QRectF shrunk = rect.adjusted(distance, distance, -distance, -distance);
        const bool shrunkValid = shrunk.width() > 0.0 && shrunk.height() > 0.0;

        QRectF chosen = grown;
        if (shrunkValid) {
            if (rectOutlineDistanceTo(shrunk, cursor) < rectOutlineDistanceTo(grown, cursor))
                chosen = shrunk;
        } else if (rect.contains(cursor)) {
            return {}; // the cursor asks for the inward side, but it erodes away
        }

        GraphicsPrimitive *result = clonePrimitive(source, sheetScene);
        if (!result)
            return {};
        result->setControlPoint(0, chosen.topLeft());
        result->setControlPoint(1, chosen.bottomRight());
        return { result };
    }

    // Everything else is a point chain: lines and Beziers (their control
    // polygon - a close parallel for the gentle curves schematics use),
    // polygons, and complex curves (their vertex chain, so the spline
    // through the offset vertices parallels the original).
    QVector<QPointF> points;
    points.reserve(source->controlPointCount());
    for (int i = 0; i < source->controlPointCount(); ++i)
        points.append(source->controlPoint(i));
    const bool closed = type == GraphicsPrimitive::Polyline
            || (type == GraphicsPrimitive::Spline && source->isClosedShape());

    const QVector<QPointF> sideA = offsetChain(points, distance, closed);
    const QVector<QPointF> sideB = offsetChain(points, -distance, closed);
    const QVector<QPointF> &chosen =
            chainDistanceTo(sideA, closed, cursor) <= chainDistanceTo(sideB, closed, cursor)
            ? sideA : sideB;
    if (closed) {
        // Erosion guard: an inward offset larger than the shape flips (or
        // collapses) the winding - there is nothing sane on that side.
        const qreal original = signedAreaTwice(points);
        const qreal offset = signedAreaTwice(chosen);
        if (qFuzzyIsNull(original) || qFuzzyIsNull(offset)
                || (original > 0) != (offset > 0))
            return {};
    }

    GraphicsPrimitive *result = nullptr;
    switch (type) {
    case GraphicsPrimitive::Polyline:
        result = makeShapeLike(source, chosen, false);
        break;
    case GraphicsPrimitive::Spline:
        result = makeShapeLike(source, chosen, true, closed);
        if (result)
            copyArrowAttributes(result, source);
        break;
    case GraphicsPrimitive::Line:
    case GraphicsPrimitive::Bezier:
        result = clonePrimitive(source, sheetScene);
        if (result) {
            for (int i = 0; i < chosen.size() && i < result->controlPointCount(); ++i)
                result->setControlPoint(i, chosen.at(i));
        }
        break;
    default:
        break;
    }
    return result ? QList<GraphicsPrimitive *>{ result } : QList<GraphicsPrimitive *>();
}

// AutoCAD-style offset: the dialog asks only the distance (and whether to
// keep the original); the side comes from the mouse afterwards - a
// half-transparent live preview of the result follows the cursor's side of
// the shape, and nothing is committed until the click. Chain offsets are
// exact miter parallels with the same node count as the original.
void MainWindow::clickOffsetOutlineAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() != 1 || !offsettable(selected.first()))
        return;
    GraphicsPrimitive *source = selected.first();

    SettingsManager &settings = SettingsManager::getInstance();
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Offset outline"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *spinDistance = new QDoubleSpinBox(&dialog);
    spinDistance->setRange(0.1, 500.0);
    spinDistance->setDecimals(1);
    spinDistance->setValue(5.0);
    form->addRow(tr("Distance (drawing units):"), spinDistance);
    layout->addLayout(form);
    auto *checkKeepOriginal = new QCheckBox(tr("Keep the original shape"), &dialog);
    checkKeepOriginal->setChecked(settings.loadSetting("offset_keep_original").toBool());
    layout->addWidget(checkKeepOriginal);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    spinDistance->setFocus();
    spinDistance->selectAll();

    if (dialog.exec() != QDialog::Accepted)
        return;
    const double distance = spinDistance->value();
    const bool keepOriginal = checkKeepOriginal->isChecked();
    settings.saveSetting("offset_keep_original", keepOriginal);

    // Interactive side pick: the preview is real primitives at half
    // opacity, rebuilt as the cursor changes side, never on the undo stack.
    QList<GraphicsPrimitive *> preview;
    auto clearPreview = [this, &preview]() {
        for (GraphicsPrimitive *primitive : std::as_const(preview))
            sheetScene->removePrimitive(primitive);
        preview.clear();
    };

    ui->statusbar->showMessage(
            tr("Move the mouse to the side to offset toward, then click (right click or Esc cancels)"), 0);
    QPointF picked;
    ScenePointPicker picker(activeView(), sheetScene, false);
    picker.setHoverCallback([this, source, distance, &preview, &clearPreview](const QPointF &pos) {
        clearPreview();
        preview = buildOffsetPrimitives(source, distance, pos);
        for (GraphicsPrimitive *primitive : std::as_const(preview)) {
            primitive->setOpacity(0.55);
            sheetScene->addPrimitive(primitive);
        }
    });
    const bool accepted = picker.run(&picked);
    clearPreview();
    ui->statusbar->clearMessage();
    if (!accepted)
        return;
    // The source may have been destroyed inside the picker's nested event
    // loop (global shortcuts stay live there) - same guard as Split.
    if (!sheetScene->primitives().contains(source))
        return;

    const QList<GraphicsPrimitive *> results = buildOffsetPrimitives(source, distance, picked);
    if (results.isEmpty()) {
        ui->statusbar->showMessage(tr("The offset on that side would erode the shape away"), 4000);
        return;
    }

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Offset outline"));
    if (!keepOriginal)
        undo->push(new DeletePrimitiveCommand(sheetScene, source));
    for (GraphicsPrimitive *result : results) {
        undo->push(new CreatePrimitiveCommand(sheetScene, result));
        result->setSelected(true);
    }
    undo->endMacro();
}

void MainWindow::clickChamferCornersAction()
{
    roundSelectedCorners(true);
}

namespace {

// The hatch segments filling `region` with parallel lines at `angleDeg`
// (counterclockwise from horizontal, in the y-grows-down scene space)
// spaced `spacing` units apart. Classic scanline clipping: each candidate
// line is intersected with every polygon edge of the region, the crossing
// points are sorted along the line, and consecutive pairs bound an inside
// stretch (even-odd rule - which also carves holes, e.g. a boolean
// "keyhole" polygon, for free). Nearly-coincident crossings (the line
// grazing a vertex, which would otherwise unbalance the pairing) collapse
// into one.
QList<QLineF> hatchSegments(const QPainterPath &region, qreal angleDeg, qreal spacing)
{
    QList<QLineF> segments;
    const QRectF bounds = region.boundingRect();
    if (bounds.isEmpty() || spacing <= 0)
        return segments;

    const qreal radians = qDegreesToRadians(angleDeg);
    // Negative y component: scene y grows downwards, and the dialog's angle
    // should read the usual way (45 = rising to the right).
    const QPointF dir(std::cos(radians), -std::sin(radians));
    const QPointF normal(-dir.y(), dir.x());

    // Scan range: project the bounding rect's corners onto the normal.
    const QPointF center = bounds.center();
    const QPointF corners[4] = { bounds.topLeft(), bounds.topRight(),
                                 bounds.bottomLeft(), bounds.bottomRight() };
    qreal minOffset = 0.0, maxOffset = 0.0;
    for (int i = 0; i < 4; ++i) {
        const QPointF d = corners[i] - center;
        const qreal offset = d.x() * normal.x() + d.y() * normal.y();
        minOffset = i == 0 ? offset : qMin(minOffset, offset);
        maxOffset = i == 0 ? offset : qMax(maxOffset, offset);
    }
    // Long enough to cross the whole bounding rect at any angle.
    const qreal halfLength = std::hypot(bounds.width(), bounds.height());

    // The region's outline as closed rings.
    QList<QPolygonF> rings = region.toSubpathPolygons();
    for (QPolygonF &ring : rings) {
        if (!ring.isEmpty() && ring.first() != ring.last())
            ring.append(ring.first());
    }

    for (qreal offset = std::ceil(minOffset / spacing) * spacing;
            offset <= maxOffset; offset += spacing) {
        const QPointF base = center + normal * offset;
        const QPointF a = base - dir * halfLength;
        const QPointF b = base + dir * halfLength;
        const QLineF scan(a, b);
        const QPointF ab = b - a;
        const qreal abLengthSq = QPointF::dotProduct(ab, ab);

        QVector<qreal> crossings;
        for (const QPolygonF &ring : rings) {
            for (int i = 0; i + 1 < ring.size(); ++i) {
                const QLineF edge(ring.at(i), ring.at(i + 1));
                QPointF intersection;
                if (scan.intersects(edge, &intersection) == QLineF::BoundedIntersection)
                    crossings.append(QPointF::dotProduct(intersection - a, ab) / abLengthSq);
            }
        }
        std::sort(crossings.begin(), crossings.end());
        // Collapse vertex-grazing duplicates (see the comment above).
        QVector<qreal> unique;
        for (qreal t : crossings) {
            if (unique.isEmpty() || (t - unique.last()) * halfLength * 2 > 0.01)
                unique.append(t);
        }
        for (int i = 0; i + 1 < unique.size(); i += 2) {
            const QPointF p1 = a + ab * unique.at(i);
            const QPointF p2 = a + ab * unique.at(i + 1);
            if (QLineF(p1, p2).length() > 0.01)
                segments.append(QLineF(p1, p2));
        }
    }
    return segments;
}

} // namespace

void MainWindow::clickHatchAction()
{
    QList<GraphicsPrimitive *> shapes;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        if (primitive->supportsBooleanOps())
            shapes.append(primitive);
    }
    if (shapes.isEmpty())
        return;

    DialogHatch dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QList<qreal> angles = { dialog.angleDeg() };
    if (dialog.crossHatch())
        angles.append(dialog.angleDeg() + 90.0);

    // All segments first, so a runaway combination (huge shape, tiny
    // spacing) can be refused before anything reaches the undo stack.
    QList<QPair<GraphicsPrimitive *, QLineF>> generated;
    for (GraphicsPrimitive *shape : shapes) {
        const QPainterPath region = shape->booleanOutline();
        for (qreal angle : angles) {
            const QList<QLineF> segments = hatchSegments(region, angle, dialog.spacing());
            for (const QLineF &segment : segments)
                generated.append({ shape, segment });
        }
    }
    if (generated.isEmpty())
        return;
    if (generated.size() > 5000) {
        QMessageBox::warning(this, tr("Hatch"),
                tr("This hatch would create %1 lines - increase the spacing and try again.")
                        .arg(generated.size()));
        return;
    }

    // Plain LI primitives on the shape's own layer, in one undo step - the
    // outline itself stays untouched, and the file format stays exactly
    // FidoCadJ's.
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Hatch"));
    for (const auto &entry : generated) {
        auto *line = new PrimitiveLine();
        line->setControlPoint(0, entry.second.p1());
        line->setControlPoint(1, entry.second.p2());
        line->setLayer(entry.first->layer());
        undo->push(new CreatePrimitiveCommand(sheetScene, line));
    }
    undo->endMacro();
}

// Splits the single selected line-like primitive in two at a clicked point:
// a line/track at the point's projection onto it, a Bezier by de Casteljau
// subdivision at the nearest curve parameter, an open complex curve at the
// point's projection onto its vertex chain. The two halves are fresh
// primitives replacing the original (delete+create undo macro); the arrow
// that would land at the joint is dropped on each side, and only the first
// half keeps the name/value label so it doesn't end up duplicated.
void MainWindow::clickSplitAtPointAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() != 1 || !splittableAtPoint(selected.first()))
        return;
    GraphicsPrimitive *primitive = selected.first();

    ui->statusbar->showMessage(tr("Click the split point (right click or Esc cancels)"), 0);
    QPointF picked;
    ScenePointPicker picker(activeView(), sheetScene);
    const bool accepted = picker.run(&picked);
    ui->statusbar->clearMessage();
    if (!accepted)
        return;
    // Global shortcuts stay live inside the picker's nested event loop -
    // Ctrl+Z past the primitive's creation, Del, or File > New can have
    // destroyed it while we were waiting for the click. Revalidate before
    // touching the captured pointer.
    if (!sheetScene->primitives().contains(primitive))
        return;

    GraphicsPrimitive *first = nullptr;
    GraphicsPrimitive *second = nullptr;
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Line:
    case GraphicsPrimitive::PcbTrack: {
        const QPointF a = primitive->controlPoint(0);
        const QPointF b = primitive->controlPoint(1);
        const qreal t = segmentParam(picked, a, b);
        if (t < 0.001 || t > 0.999)
            break; // the pick landed on (or past) an endpoint - nothing to split
        const QPointF x = a + (b - a) * t;
        first = clonePrimitive(primitive, sheetScene);
        second = clonePrimitive(primitive, sheetScene);
        if (!first || !second)
            break;
        first->setControlPoint(1, x);
        second->setControlPoint(0, x);
        break;
    }
    case GraphicsPrimitive::Bezier: {
        const QPointF p0 = primitive->controlPoint(0);
        const QPointF p1 = primitive->controlPoint(1);
        const QPointF p2 = primitive->controlPoint(2);
        const QPointF p3 = primitive->controlPoint(3);
        auto cubicAt = [&](qreal t) {
            const qreal u = 1.0 - t;
            return u * u * u * p0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t * p3;
        };
        // The curve parameter nearest the pick, by dense sampling - plenty
        // for a click-driven split.
        qreal bestT = 0.5;
        qreal bestDistSq = std::numeric_limits<qreal>::max();
        constexpr int Samples = 256;
        for (int i = 1; i < Samples; ++i) {
            const qreal t = qreal(i) / Samples;
            const QPointF diff = cubicAt(t) - picked;
            const qreal distSq = QPointF::dotProduct(diff, diff);
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestT = t;
            }
        }
        if (bestT < 0.01 || bestT > 0.99)
            break;
        // De Casteljau subdivision: both halves together retrace the
        // original curve exactly.
        const QPointF q0 = p0 + (p1 - p0) * bestT;
        const QPointF q1 = p1 + (p2 - p1) * bestT;
        const QPointF q2 = p2 + (p3 - p2) * bestT;
        const QPointF r0 = q0 + (q1 - q0) * bestT;
        const QPointF r1 = q1 + (q2 - q1) * bestT;
        const QPointF s = r0 + (r1 - r0) * bestT;
        first = clonePrimitive(primitive, sheetScene);
        second = clonePrimitive(primitive, sheetScene);
        if (!first || !second)
            break;
        first->setControlPoint(1, q0);
        first->setControlPoint(2, r0);
        first->setControlPoint(3, s);
        second->setControlPoint(0, s);
        second->setControlPoint(1, r1);
        second->setControlPoint(2, q2);
        break;
    }
    case GraphicsPrimitive::Spline: {
        const int insertIndex = nearestSegmentInsertIndex(primitive, picked);
        const QPointF a = primitive->controlPoint(insertIndex - 1);
        const QPointF b = primitive->controlPoint(insertIndex);
        const QPointF x = a + (b - a) * segmentParam(picked, a, b);
        QVector<QPointF> firstVertices, secondVertices;
        for (int i = 0; i < insertIndex; ++i)
            firstVertices.append(primitive->controlPoint(i));
        firstVertices.append(x);
        secondVertices.append(x);
        for (int i = insertIndex; i < primitive->controlPointCount(); ++i)
            secondVertices.append(primitive->controlPoint(i));
        first = makeShapeLike(primitive, firstVertices, true, false);
        second = makeShapeLike(primitive, secondVertices, true, false);
        if (first)
            copyArrowAttributes(first, primitive);
        if (second)
            copyArrowAttributes(second, primitive);
        break;
    }
    default:
        break;
    }

    if (!first || !second) {
        delete first;
        delete second;
        ui->statusbar->showMessage(tr("The point does not split the primitive"), 4000);
        return;
    }
    first->setArrowAtEnd(false);
    second->setArrowAtStart(false);
    second->setName(QString());
    second->setValue(QString());

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Split at point"));
    undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    // push() calls redo() synchronously, which is what actually adds the
    // primitive to the scene - only safe to select it afterwards.
    undo->push(new CreatePrimitiveCommand(sheetScene, first));
    first->setSelected(true);
    undo->push(new CreatePrimitiveCommand(sheetScene, second));
    second->setSelected(true);
    undo->endMacro();
}

// The line/track nearest `scenePos` within a few screen pixels - the click
// target both trim and extend operate on. Straight two-point primitives
// only: they're the ones with a well-defined "portion between
// intersections" and "direction to extend along".
GraphicsPrimitive *MainWindow::pickedLineAt(const QPointF &scenePos) const
{
    const qreal tolerance = 8.0 / qMax(0.01, activeView()->transform().m11());
    GraphicsPrimitive *best = nullptr;
    qreal bestDistSq = tolerance * tolerance;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        const GraphicsPrimitive::PrimitiveTypes type = primitive->getPrimitiveType();
        if (type != GraphicsPrimitive::Line && type != GraphicsPrimitive::PcbTrack)
            continue;
        if (!primitive->isOnCanvas())
            continue;
        const qreal distSq = pointToSegmentDistanceSq(scenePos,
                                                      primitive->controlPoint(0),
                                                      primitive->controlPoint(1));
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            best = primitive;
        }
    }
    return best;
}

// AutoCAD-style Trim: the clicked stretch of a line/track - between its
// intersections with whatever other primitives cross it - is removed. A
// clicked end stretch just shortens the line (an undoable endpoint move); a
// clicked middle stretch splits it in two around the removed part. While
// hunting for the click, the stretch that would be removed is previewed
// live as a dashed red overlay, AutoCAD-style.
void MainWindow::clickTrimToIntersectionAction()
{
    // What clicking at `pos` would remove: the containing stretch
    // [lower, upper] (parameters along the target's [a, b]) between the
    // target's crossings with every other primitive's outline. nullopt when
    // there's no line there or nothing crosses it.
    struct TrimPlan {
        GraphicsPrimitive *target;
        QPointF a, b;
        qreal lower, upper;
    };
    auto computePlan = [this](const QPointF &pos) -> std::optional<TrimPlan> {
        GraphicsPrimitive *target = pickedLineAt(pos);
        if (!target)
            return std::nullopt;
        const QPointF a = target->controlPoint(0);
        const QPointF b = target->controlPoint(1);

        QVector<qreal> cuts;
        for (GraphicsPrimitive *other : sheetScene->primitives()) {
            if (other == target || !other->isOnCanvas())
                continue;
            const QList<QPolygonF> polylines = boundaryPolylines(other);
            for (const QPolygonF &polyline : polylines) {
                for (int i = 0; i + 1 < polyline.size(); ++i) {
                    QPointF crossing;
                    if (QLineF(a, b).intersects(QLineF(polyline.at(i), polyline.at(i + 1)),
                                                &crossing) == QLineF::BoundedIntersection) {
                        const qreal t = segmentParam(crossing, a, b);
                        if (t > 1e-6 && t < 1.0 - 1e-6)
                            cuts.append(t);
                    }
                }
            }
        }
        if (cuts.isEmpty())
            return std::nullopt;
        std::sort(cuts.begin(), cuts.end());

        const qreal tClick = segmentParam(pos, a, b);
        qreal lower = 0.0, upper = 1.0;
        for (qreal t : std::as_const(cuts)) {
            if (t <= tClick)
                lower = t;
            else {
                upper = t;
                break;
            }
        }
        return TrimPlan{ target, a, b, lower, upper };
    };

    // The would-be-removed stretch, previewed in red. Sheet foreground
    // state, deliberately not a QGraphicsItem: if the document is replaced
    // while the pick's nested event loop runs (Ctrl+N is still live), the
    // scene's clear() would destroy an overlay item and leave this function
    // holding a dangling pointer - plain state can't dangle.
    ui->statusbar->showMessage(
            tr("Click the part of a line to remove (right click or Esc cancels)"), 0);
    QPointF picked;
    ScenePointPicker picker(activeView(), sheetScene, false);
    picker.setHoverCallback([this, &computePlan](const QPointF &pos) {
        if (const auto plan = computePlan(pos)) {
            sheetScene->setHoverHighlightLine(
                    QLineF(plan->a + (plan->b - plan->a) * plan->lower,
                           plan->a + (plan->b - plan->a) * plan->upper),
                    QColor(220, 40, 40));
        } else {
            sheetScene->clearHoverHighlight();
        }
    });
    const bool accepted = picker.run(&picked);
    sheetScene->clearPickHighlights();
    ui->statusbar->clearMessage();
    if (!accepted)
        return;

    const auto plan = computePlan(picked);
    if (!plan) {
        ui->statusbar->showMessage(pickedLineAt(picked)
                ? tr("Nothing crosses that line - nothing to trim")
                : tr("No line or PCB track there"), 4000);
        return;
    }
    GraphicsPrimitive *target = plan->target;
    const QPointF a = plan->a;
    const QPointF b = plan->b;
    const qreal lower = plan->lower;
    const qreal upper = plan->upper;

    QUndoStack *undo = sheetScene->undoStack();
    if (lower <= 0.0 || upper >= 1.0) {
        // An end portion - the line just gets shorter: a plain endpoint
        // move, undoable like any drag.
        const QVector<QPointF> before = target->controlPointSnapshot();
        if (lower <= 0.0)
            target->setControlPoint(0, a + (b - a) * upper);
        else
            target->setControlPoint(1, a + (b - a) * lower);
        undo->push(new MovePrimitiveCommand(target, before,
                                            target->controlPointSnapshot()));
        return;
    }

    // A middle portion - the line splits into the two stretches around it.
    GraphicsPrimitive *first = clonePrimitive(target, sheetScene);
    GraphicsPrimitive *second = clonePrimitive(target, sheetScene);
    if (!first || !second) {
        delete first;
        delete second;
        return;
    }
    first->setControlPoint(1, a + (b - a) * lower);
    first->setArrowAtEnd(false);
    second->setControlPoint(0, a + (b - a) * upper);
    second->setArrowAtStart(false);
    second->setName(QString());
    second->setValue(QString());

    sheetScene->clearSelection();
    undo->beginMacro(tr("Trim to intersection"));
    undo->push(new DeletePrimitiveCommand(sheetScene, target));
    undo->push(new CreatePrimitiveCommand(sheetScene, first));
    undo->push(new CreatePrimitiveCommand(sheetScene, second));
    undo->endMacro();
}

// AutoCAD-style Extend: the clicked line's nearest endpoint slides outward
// along the line's own direction until it meets the first primitive outline
// on its way - an undoable endpoint move. While hunting for the click, the
// stretch that would be added is previewed live as a dashed green overlay.
void MainWindow::clickExtendToIntersectionAction()
{
    // What clicking at `pos` would do: move `target`'s endpoint
    // controlPoint(endIndex) (currently at `end`) out to `reached`.
    struct ExtendPlan {
        GraphicsPrimitive *target;
        int endIndex;
        QPointF end, reached;
    };
    auto computePlan = [this](const QPointF &pos) -> std::optional<ExtendPlan> {
        GraphicsPrimitive *target = pickedLineAt(pos);
        if (!target)
            return std::nullopt;
        const QPointF a = target->controlPoint(0);
        const QPointF b = target->controlPoint(1);
        const int endIndex = segmentParam(pos, a, b) < 0.5 ? 0 : 1;
        const QPointF end = endIndex == 0 ? a : b;
        const QPointF inner = endIndex == 0 ? b : a;
        const QLineF span(inner, end);
        if (span.length() <= 0.0)
            return std::nullopt;
        const QPointF direction = (end - inner) / span.length();

        // First outline crossing strictly beyond the endpoint, along the ray.
        qreal bestDistance = std::numeric_limits<qreal>::max();
        QPointF bestPoint;
        for (GraphicsPrimitive *other : sheetScene->primitives()) {
            if (other == target || !other->isOnCanvas())
                continue;
            const QList<QPolygonF> polylines = boundaryPolylines(other);
            for (const QPolygonF &polyline : polylines) {
                for (int i = 0; i + 1 < polyline.size(); ++i) {
                    const QLineF edge(polyline.at(i), polyline.at(i + 1));
                    QPointF crossing;
                    if (QLineF(end, end + direction).intersects(edge, &crossing)
                            == QLineF::NoIntersection)
                        continue;
                    // The infinite-line crossing must actually lie on the
                    // edge...
                    const QPointF edgeVector = edge.p2() - edge.p1();
                    const qreal edgeLengthSq = QPointF::dotProduct(edgeVector, edgeVector);
                    if (edgeLengthSq <= 0.0)
                        continue;
                    const qreal u = QPointF::dotProduct(crossing - edge.p1(), edgeVector)
                            / edgeLengthSq;
                    if (u < 0.0 || u > 1.0)
                        continue;
                    // ...and ahead of the endpoint, not behind it.
                    const qreal distance = QPointF::dotProduct(crossing - end, direction);
                    if (distance > 1e-6 && distance < bestDistance) {
                        bestDistance = distance;
                        bestPoint = crossing;
                    }
                }
            }
        }
        if (bestDistance == std::numeric_limits<qreal>::max())
            return std::nullopt;
        return ExtendPlan{ target, endIndex, end, bestPoint };
    };

    // The would-be-added stretch, previewed in green - Sheet foreground
    // state, not a QGraphicsItem, for the same document-replaced-mid-pick
    // reason as the trim preview above.
    ui->statusbar->showMessage(
            tr("Click a line near the end to extend (right click or Esc cancels)"), 0);
    QPointF picked;
    ScenePointPicker picker(activeView(), sheetScene, false);
    picker.setHoverCallback([this, &computePlan](const QPointF &pos) {
        if (const auto plan = computePlan(pos)) {
            sheetScene->setHoverHighlightLine(QLineF(plan->end, plan->reached),
                                              QColor(30, 170, 60));
        } else {
            sheetScene->clearHoverHighlight();
        }
    });
    const bool accepted = picker.run(&picked);
    sheetScene->clearPickHighlights();
    ui->statusbar->clearMessage();
    if (!accepted)
        return;

    const auto plan = computePlan(picked);
    if (!plan) {
        ui->statusbar->showMessage(pickedLineAt(picked)
                ? tr("Nothing to extend to in that direction")
                : tr("No line or PCB track there"), 4000);
        return;
    }

    const QVector<QPointF> before = plan->target->controlPointSnapshot();
    plan->target->setControlPoint(plan->endIndex, plan->reached);
    sheetScene->undoStack()->push(
            new MovePrimitiveCommand(plan->target, before,
                                     plan->target->controlPointSnapshot()));
}

// Scales every control point (labels included - they travel with the shape,
// like a move) around the first selected primitive's first point, matching
// Mirror/Rotate's pivot convention. Per-primitive scalar sizes (text
// character size, pad size, track width) are deliberately left alone.
void MainWindow::clickScaleSelectionAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    bool ok = false;
    const double percent = QInputDialog::getDouble(
                this, tr("Scale"), tr("Scale factor (%):"), 100.0, 1.0, 10000.0, 1, &ok);
    if (!ok || qFuzzyCompare(percent, 100.0))
        return;
    const qreal factor = percent / 100.0;
    const QPointF pivot = selected.first()->controlPoint(0);

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Scale"));
    for (GraphicsPrimitive *primitive : selected) {
        const QVector<QPointF> before = primitive->controlPointSnapshot();
        QVector<QPointF> after;
        after.reserve(before.size());
        for (const QPointF &point : before)
            after.append(pivot + (point - pivot) * factor);
        primitive->restoreControlPoints(after);
        undo->push(new MovePrimitiveCommand(primitive, before, after));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickMoveBasePointAction()
{
    moveOrCopyWithBasePoint(false);
}

void MainWindow::clickCopyBasePointAction()
{
    moveOrCopyWithBasePoint(true);
}

// AutoCAD's Move/Copy gesture: "specify base point" then "specify
// destination" - both picks fully snapped (object snap and tracking
// included), so a vertex can be carried exactly onto another primitive's
// vertex. Between the two picks a half-opacity copy of the selection
// follows the cursor, offset by the live displacement.
void MainWindow::moveOrCopyWithBasePoint(bool copyMode)
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    ui->statusbar->showMessage(
            tr("Specify the base point (right click or Esc cancels)"), 0);
    QPointF base;
    bool accepted = false;
    {
        ScenePointPicker basePicker(activeView(), sheetScene, true);
        accepted = basePicker.run(&base);
    }
    if (!accepted) {
        ui->statusbar->clearMessage();
        return;
    }
    // The selection may have been altered inside the picker's nested event
    // loop (global shortcuts stay live there) - same guard as Split/Offset.
    for (GraphicsPrimitive *primitive : selected) {
        if (!sheetScene->primitives().contains(primitive)) {
            ui->statusbar->clearMessage();
            return;
        }
    }

    // Live preview: one half-opacity clone per selected primitive, cloned
    // once here and only *translated* per mouse move - never rebuilt, so
    // even a large selection previews cheaply. Never on the undo stack.
    QList<GraphicsPrimitive *> preview;
    QList<QVector<QPointF>> previewBase;
    // The preview clones are real primitives on the sheet, so they must be
    // excluded from the destination pick's snapping - their vertices track
    // the cursor and would otherwise be captured (and snap-tracking
    // acquired) instead of the actual drawing's points.
    QList<const GraphicsPrimitive *> previewExcluded;
    for (GraphicsPrimitive *primitive : selected) {
        if (GraphicsPrimitive *clone = clonePrimitive(primitive, sheetScene)) {
            clone->setOpacity(0.55);
            sheetScene->addPrimitive(clone);
            preview.append(clone);
            previewBase.append(clone->controlPointSnapshot());
            previewExcluded.append(clone);
        }
    }
    auto clearPreview = [this, &preview]() {
        for (GraphicsPrimitive *primitive : std::as_const(preview))
            sheetScene->removePrimitive(primitive);
        preview.clear();
    };

    ui->statusbar->showMessage(
            tr("Specify the destination point (right click or Esc cancels)"), 0);
    QPointF destination;
    ScenePointPicker destinationPicker(activeView(), sheetScene, true);
    destinationPicker.setExcluded(previewExcluded);
    destinationPicker.setHoverCallback([this, base, &preview, &previewBase,
                                        &previewExcluded](const QPointF &pos) {
        // Preview against the snapped position, so what's shown is exactly
        // where the click would land.
        const QPointF delta = sheetScene->snapPosition(pos, previewExcluded) - base;
        for (int i = 0; i < preview.size(); ++i) {
            QVector<QPointF> moved = previewBase.at(i);
            for (QPointF &point : moved)
                point += delta;
            preview.at(i)->restoreControlPoints(moved);
        }
    });
    accepted = destinationPicker.run(&destination);
    clearPreview();
    ui->statusbar->clearMessage();
    if (!accepted)
        return;
    for (GraphicsPrimitive *primitive : selected) {
        if (!sheetScene->primitives().contains(primitive))
            return;
    }

    const QPointF delta = destination - base;
    if (delta.isNull())
        return;

    if (copyMode) {
        sheetScene->clearSelection();
        QUndoStack *undo = sheetScene->undoStack();
        undo->beginMacro(tr("Copy with base point"));
        for (GraphicsPrimitive *primitive : selected) {
            GraphicsPrimitive *copy = clonePrimitive(primitive, sheetScene);
            if (!copy)
                continue;
            copy->translateControlPoints(delta);
            undo->push(new CreatePrimitiveCommand(sheetScene, copy));
            copy->setSelected(true);
        }
        undo->endMacro();
    } else {
        QHash<GraphicsPrimitive *, QPointF> deltas;
        for (GraphicsPrimitive *primitive : selected)
            deltas.insert(primitive, delta);
        moveSelectedPrimitives(deltas, tr("Move with base point"));
    }
}

// Rotates the selection by an arbitrary angle around the first selected
// primitive's first point. Point-based primitives rotate exactly; a
// rectangle/ellipse can't (the format keeps them axis-aligned), so they are
// first rewritten as a polygon/closed complex curve and then rotated - same
// delete+create undo step as an explicit conversion.
void MainWindow::clickRotateByAngleAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    bool ok = false;
    const double degrees = QInputDialog::getDouble(
                this, tr("Rotate by angle"), tr("Angle (degrees, counterclockwise):"),
                45.0, -360.0, 360.0, 1, &ok);
    if (!ok || qFuzzyIsNull(degrees))
        return;

    // Y grows downward on the sheet, so a visually counterclockwise rotation
    // needs the sign of the sine flipped relative to the textbook formula.
    const qreal radians = qDegreesToRadians(degrees);
    const qreal cosine = std::cos(radians);
    const qreal sine = std::sin(radians);
    const QPointF pivot = selected.first()->controlPoint(0);
    auto rotated = [&](const QPointF &point) {
        const QPointF delta = point - pivot;
        return pivot + QPointF(delta.x() * cosine + delta.y() * sine,
                               -delta.x() * sine + delta.y() * cosine);
    };

    QUndoStack *undo = sheetScene->undoStack();
    undo->beginMacro(tr("Rotate by angle"));
    for (GraphicsPrimitive *primitive : selected) {
        const GraphicsPrimitive::PrimitiveTypes type = primitive->getPrimitiveType();
        if (type == GraphicsPrimitive::Rectangle || type == GraphicsPrimitive::Ellipse) {
            const bool asCurve = type == GraphicsPrimitive::Ellipse;
            QVector<QPointF> vertices = conversionVertices(primitive, asCurve);
            for (QPointF &vertex : vertices)
                vertex = rotated(vertex);
            GraphicsPrimitive *replacement = makeShapeLike(primitive, vertices, asCurve);
            if (!replacement)
                continue;
            primitive->setSelected(false);
            undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
            undo->push(new CreatePrimitiveCommand(sheetScene, replacement));
            replacement->setSelected(true);
        } else {
            const QVector<QPointF> before = primitive->controlPointSnapshot();
            QVector<QPointF> after;
            after.reserve(before.size());
            for (const QPointF &point : before)
                after.append(rotated(point));
            primitive->restoreControlPoints(after);
            undo->push(new MovePrimitiveCommand(primitive, before, after));
        }
    }
    undo->endMacro();
}

// Replicates the selection either on a grid (a fresh parse of the selection
// dropped at each cell's offset - same serialize/re-parse round trip
// Duplicate uses) or circularly around a center, all as one undo step.
void MainWindow::clickArrayAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    QRectF bounds;
    for (GraphicsPrimitive *primitive : selected)
        bounds = bounds.united(primitive->sceneBoundingRect());

    DialogArray dialog(this);
    dialog.setSuggestedCenter(bounds.center());
    // "Pick from canvas" closes the dialog with PickRequested; the pick runs
    // here with no modal window in the way, then the same dialog instance is
    // reopened with every field (and the picked center) preserved.
    int result = dialog.exec();
    while (result == DialogArray::PickRequested) {
        ScenePointPicker picker(activeView(), sheetScene);
        QPointF picked;
        if (picker.run(&picked))
            dialog.setSuggestedCenter(picked);
        result = dialog.exec();
    }
    if (result != QDialog::Accepted)
        return;

    const QString serialized = FidoCadWriter::writeSelection(selected);
    QUndoStack *undo = sheetScene->undoStack();

    if (dialog.mode() == DialogArray::Mode::Grid) {
        const int columns = dialog.columns();
        const int rows = dialog.rows();
        if (columns * rows <= 1)
            return;

        undo->beginMacro(tr("Array of copies"));
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < columns; ++column) {
                if (row == 0 && column == 0)
                    continue; // that's the original itself
                const QPointF offset(column * dialog.spacingX(), row * dialog.spacingY());
                const QList<GraphicsPrimitive *> copies = FidoCadReader::parse(serialized, sheetScene);
                for (GraphicsPrimitive *copy : copies) {
                    copy->translateControlPoints(offset);
                    undo->push(new CreatePrimitiveCommand(sheetScene, copy));
                    copy->setSelected(true);
                }
            }
        }
        undo->endMacro();
        return;
    }

    // Circular: `total` instances (the original included) spread over the
    // requested angle. A full 360° divides by the count (first and last
    // must not overlap); a partial arc divides by count-1 so its ends land
    // exactly on the arc's bounds - AutoCAD's own polar-array convention.
    const int total = dialog.copies();
    if (total < 2)
        return;
    const qreal fill = dialog.totalAngle();
    const bool fullCircle = qFuzzyCompare(qAbs(fill), 360.0);
    const qreal stepDegrees = fullCircle ? fill / total : fill / (total - 1);
    const QPointF center = dialog.center();
    const bool rotateCopies = dialog.rotateCopies();
    const QPointF anchor = bounds.center();

    undo->beginMacro(tr("Array of copies"));
    for (int i = 1; i < total; ++i) {
        const qreal radians = qDegreesToRadians(stepDegrees * i);
        const qreal cosine = std::cos(radians);
        const qreal sine = std::sin(radians);
        // Visual counterclockwise on the y-down sheet, like Rotate by angle.
        auto rotated = [&](const QPointF &point) {
            const QPointF delta = point - center;
            return center + QPointF(delta.x() * cosine + delta.y() * sine,
                                    -delta.x() * sine + delta.y() * cosine);
        };
        const QList<GraphicsPrimitive *> copies = FidoCadReader::parse(serialized, sheetScene);
        for (GraphicsPrimitive *copy : copies) {
            GraphicsPrimitive *instance = copy;
            if (rotateCopies) {
                const GraphicsPrimitive::PrimitiveTypes type = copy->getPrimitiveType();
                if (type == GraphicsPrimitive::Rectangle || type == GraphicsPrimitive::Ellipse) {
                    // Axis-aligned in the format - rewritten as a polygon/
                    // closed curve before rotating, same as Rotate by angle.
                    const bool asCurve = type == GraphicsPrimitive::Ellipse;
                    QVector<QPointF> vertices = conversionVertices(copy, asCurve);
                    for (QPointF &vertex : vertices)
                        vertex = rotated(vertex);
                    if (GraphicsPrimitive *replacement = makeShapeLike(copy, vertices, asCurve)) {
                        delete copy; // never added to the scene - plain delete is safe
                        instance = replacement;
                    }
                } else {
                    const QVector<QPointF> before = copy->controlPointSnapshot();
                    QVector<QPointF> after;
                    after.reserve(before.size());
                    for (const QPointF &point : before)
                        after.append(rotated(point));
                    copy->restoreControlPoints(after);
                }
            } else {
                // Keep each copy's own orientation: only its position (the
                // selection's center) travels along the circle.
                instance->translateControlPoints(rotated(anchor) - anchor);
            }
            undo->push(new CreatePrimitiveCommand(sheetScene, instance));
            instance->setSelected(true);
        }
    }
    undo->endMacro();
}

// Rounds every point of the selection to the configured snap step - for
// tidying up primitives drawn with snapping off or imported from DXF.
// Deliberately ignores the "snap_enabled" toggle (being off is precisely
// why things ended up off-grid) and uses the raw step instead.
void MainWindow::clickSnapSelectionToGridAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    const int step = qMax(1, SettingsManager::getInstance().loadSetting("snap_step").toInt());
    auto snapped = [step](const QPointF &point) {
        return QPointF(qRound(point.x() / step) * qreal(step),
                       qRound(point.y() / step) * qreal(step));
    };

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Snap selection to grid"));
    for (GraphicsPrimitive *primitive : selected) {
        const QVector<QPointF> before = primitive->controlPointSnapshot();
        QVector<QPointF> after;
        after.reserve(before.size());
        for (const QPointF &point : before)
            after.append(snapped(point));
        if (after == before)
            continue;
        primitive->restoreControlPoints(after);
        undo->push(new MovePrimitiveCommand(primitive, before, after));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickSelectSameTypeAction()
{
    QSet<int> selectedTypes;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder())
        selectedTypes.insert(primitive->getPrimitiveType());
    if (selectedTypes.isEmpty())
        return;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (selectedTypes.contains(primitive->getPrimitiveType()))
            primitive->setSelected(true);
    }
}

void MainWindow::clickInvertSelectionAction()
{
    for (GraphicsPrimitive *primitive : sheetScene->primitives())
        primitive->setSelected(!primitive->isSelected());
}

void MainWindow::cloneSelectionInPlace()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    const QList<GraphicsPrimitive *> clones =
            FidoCadReader::parse(FidoCadWriter::writeSelection(selected), sheetScene);
    if (clones.isEmpty())
        return;

    // The clones stay exactly where the originals were and are deliberately
    // NOT selected - the drag that triggered this keeps moving the selected
    // originals away from them.
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = clones.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Duplicate"));
    for (GraphicsPrimitive *clone : clones)
        undo->push(new CreatePrimitiveCommand(sheetScene, clone));
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickConvertMacroToPrimitivesAction()
{
    QList<PrimitiveMacro *> macros;
    for (GraphicsPrimitive *primitive : selectedPrimitivesInOrder()) {
        if (auto *macro = dynamic_cast<PrimitiveMacro *>(primitive))
            macros.append(macro);
    }
    if (macros.isEmpty())
        return;

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    // Always grouped, even for a single macro: converting one already means
    // one delete plus N creates, and all of it must undo as one step.
    undo->beginMacro(tr("Convert macro to primitives"));
    for (PrimitiveMacro *macro : macros) {
        const QList<GraphicsPrimitive *> expanded = macro->convertToPrimitives(sheetScene);
        for (GraphicsPrimitive *primitive : expanded) {
            // push() calls redo() synchronously, which is what actually adds
            // the primitive to the scene - only safe to select it afterwards.
            undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
            primitive->setSelected(true);
        }
        undo->push(new DeletePrimitiveCommand(sheetScene, macro));
    }
    undo->endMacro();
}

void MainWindow::clickCreateMacroAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    DialogCreateMacro dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    // A fresh, independent parse of the selection (never the live canvas
    // primitives themselves) so its control points can be freely offset
    // below without touching what's actually on screen - same round-trip
    // Duplicate/Paste already rely on.
    const QString serialized = FidoCadWriter::writeSelection(selected);
    QList<GraphicsPrimitive *> clones = FidoCadReader::parse(serialized, sheetScene);
    if (clones.isEmpty())
        return;

    // Anchors the macro at the selection's top-left corner: every clone is
    // shifted so that corner lands exactly on the library format's
    // conventional (100,100) macro origin, matching how every shipped
    // library symbol is itself laid out.
    QRectF bounds;
    bool first = true;
    for (GraphicsPrimitive *clone : clones) {
        for (int i = 0; i < clone->controlPointCount(); ++i) {
            const QPointF point = clone->controlPoint(i);
            if (first) {
                bounds = QRectF(point, QSizeF(0, 0));
                first = false;
            } else {
                bounds = bounds.united(QRectF(point, QSizeF(0, 0)));
            }
        }
    }
    const QPointF offset = QPointF(100, 100) - bounds.topLeft();
    for (GraphicsPrimitive *clone : clones)
        clone->translateControlPoints(offset);
    const QString body = FidoCadWriter::writeSelection(clones);
    qDeleteAll(clones);

    // The key is never user-chosen - generated the same way the reference
    // FidoCadJ editor's own LibraryModel.createRandomMacroKey() does (a
    // short random number), retried against a collision the same way it
    // does too (up to 20 attempts, though a collision is astronomically
    // unlikely here).
    const QString libraryFilename = dialog.libraryFilename();
    QString key;
    for (int attempt = 0; attempt < 20; ++attempt) {
        key = QString::number(QRandomGenerator::global()->bounded(100000000));
        const QString fullKey = (libraryFilename + QLatin1Char('.') + key).toLower();
        if (!LibraryManager::getInstance().macro(fullKey))
            break;
    }

    QString errorMessage;
    const bool ok = LibraryManager::getInstance().addUserMacro(
                libraryFilename, dialog.libraryDisplayName(),
                key, dialog.macroName(), dialog.category(), body, &errorMessage);
    if (!ok)
        QMessageBox::warning(this, tr("Create macro"), errorMessage);
}

void MainWindow::clickDeleteAction()
{
    const QList<QGraphicsItem *> selected = sheetScene->selectedItems();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Delete"));
    for (QGraphicsItem *item : selected) {
        if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
            undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::clickSelectAllAction()
{
    for (QGraphicsItem *item : sheetScene->items())
        item->setSelected(true);
}

void MainWindow::clickFindAction()
{
    if (!findDialog) {
        findDialog = new DialogFind(this);
        connect(findDialog, &DialogFind::findNext, this, [this](const QString &text) {
            findInDrawing(text, +1);
        });
        connect(findDialog, &DialogFind::findPrevious, this, [this](const QString &text) {
            findInDrawing(text, -1);
        });
    }
    findDialog->show();
    findDialog->raise();
    findDialog->activateWindow();
}

// Searches every primitive's name()/value() (the FIDOSPECS "TY" attached
// labels, e.g. a resistor's "R1"/"10k"), PrimitiveText content, and macro
// display names (not the raw lookup key - the reader wants what's actually
// shown in the library panel) across the whole sheet, cycling forward/
// backward through the matches and wrapping around at either end. A new
// search string always restarts from the first match, matching a typical
// text editor's Ctrl+F.
void MainWindow::findInDrawing(const QString &text, int direction)
{
    if (text != lastFindText) {
        lastFindText = text;
        lastFindIndex = -1;
    }

    if (text.trimmed().isEmpty()) {
        findDialog->setStatusText(QString());
        return;
    }

    QList<GraphicsPrimitive *> matches;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        // name()/value() are the FIDOSPECS "TY" attached labels (e.g. a
        // resistor macro's "R1"/"10k") - GraphicsPrimitive base class state,
        // so checked for every primitive type, not just text/macro.
        bool matched = primitive->name().contains(text, Qt::CaseInsensitive)
                || primitive->value().contains(text, Qt::CaseInsensitive);

        if (!matched && primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
            matched = static_cast<PrimitiveText *>(primitive)->text().contains(text, Qt::CaseInsensitive);
        } else if (!matched && primitive->getPrimitiveType() == GraphicsPrimitive::PartLib) {
            auto *macroPrimitive = static_cast<PrimitiveMacro *>(primitive);
            const MacroDescriptor *descriptor = LibraryManager::getInstance().macro(macroPrimitive->macroName());
            const QString macroLabel = descriptor ? descriptor->name : macroPrimitive->macroName();
            matched = macroLabel.contains(text, Qt::CaseInsensitive);
        }

        if (matched)
            matches.append(primitive);
    }

    if (matches.isEmpty()) {
        sheetScene->clearSelection();
        findDialog->setStatusText(tr("No matches"));
        lastFindIndex = -1;
        return;
    }

    lastFindIndex = (lastFindIndex + direction + matches.size()) % matches.size();
    GraphicsPrimitive *match = matches.at(lastFindIndex);

    sheetScene->clearSelection();
    match->setSelected(true);
    activeView()->centerOn(match->sceneBoundingRect().center());

    findDialog->setStatusText(tr("%1 of %2").arg(lastFindIndex + 1).arg(matches.size()));
}

// Applies each primitive's computed delta as an undoable MovePrimitiveCommand
// (skipping primitives whose delta is a no-op), matching the reference
// FidoCadJ editor's align/distribute (EditorActions.java), which likewise
// moves each affected primitive and saves one undo state per operation.
void MainWindow::moveSelectedPrimitives(const QHash<GraphicsPrimitive *, QPointF> &deltas, const QString &undoLabel)
{
    QList<GraphicsPrimitive *> toMove;
    for (auto it = deltas.constBegin(); it != deltas.constEnd(); ++it) {
        if (!it.value().isNull())
            toMove.append(it.key());
    }
    if (toMove.isEmpty())
        return;

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = toMove.size() > 1;
    if (multiple)
        undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : toMove) {
        const QVector<QPointF> before = primitive->controlPointSnapshot();
        primitive->translateControlPoints(deltas.value(primitive));
        undo->push(new MovePrimitiveCommand(primitive, before, primitive->controlPointSnapshot()));
    }
    if (multiple)
        undo->endMacro();
}

void MainWindow::nudgeSelection(const QPointF &direction)
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    // One snap step per press (times the configurable multiplier), whether
    // or not snapping is currently enabled - the point of a keyboard nudge
    // is a predictable, grid-friendly move.
    const int multiplier = qMax(1, SettingsManager::getInstance()
                                .loadSetting("nudge_step_multiplier").toInt());
    const int step = multiplier
            * qMax(1, SettingsManager::getInstance().loadSetting("snap_step").toInt());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, direction * step);
    moveSelectedPrimitives(deltas, tr("Move selection"));
}

void MainWindow::addNodeToPrimitive(GraphicsPrimitive *primitive, const QPointF &scenePos)
{
    if (!primitive->supportsNodeEditing() || primitive->controlPointCount() < 2)
        return;
    const int index = nearestSegmentInsertIndex(primitive, scenePos);
    sheetScene->undoStack()->push(new InsertNodeCommand(primitive, index, scenePos));
}

void MainWindow::removeNodeFromPrimitive(GraphicsPrimitive *primitive, const QPointF &scenePos)
{
    if (!primitive->supportsNodeEditing() || primitive->controlPointCount() <= 2)
        return;
    const int index = nearestVertexIndex(primitive, scenePos);
    sheetScene->undoStack()->push(new RemoveNodeCommand(primitive, index));
}

// Align/distribute operate on each primitive's own bounding box (in scene
// coordinates, since pos() is always pinned at the origin - see
// GraphicsPrimitive's header comment), mirroring the reference FidoCadJ
// editor's per-primitive getPosition()/getSize() - but computed as a proper
// min/max reduction rather than replicating a latent bug in FidoCadJ's own
// getSize() that mishandles primitives with more than 2 control points
// (polygons, complex curves, macros).
void MainWindow::clickAlignLeftAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal leftmost = std::numeric_limits<qreal>::max();
    for (GraphicsPrimitive *primitive : selected)
        leftmost = qMin(leftmost, primitive->boundingRect().left());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(leftmost - primitive->boundingRect().left(), 0));
    moveSelectedPrimitives(deltas, tr("Align left"));
}

void MainWindow::clickAlignRightAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal rightmost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected)
        rightmost = qMax(rightmost, primitive->boundingRect().right());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(rightmost - primitive->boundingRect().right(), 0));
    moveSelectedPrimitives(deltas, tr("Align right"));
}

void MainWindow::clickAlignTopAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal topmost = std::numeric_limits<qreal>::max();
    for (GraphicsPrimitive *primitive : selected)
        topmost = qMin(topmost, primitive->boundingRect().top());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, topmost - primitive->boundingRect().top()));
    moveSelectedPrimitives(deltas, tr("Align top"));
}

void MainWindow::clickAlignBottomAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal bottommost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected)
        bottommost = qMax(bottommost, primitive->boundingRect().bottom());

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, bottommost - primitive->boundingRect().bottom()));
    moveSelectedPrimitives(deltas, tr("Align bottom"));
}

// Aligns every selected primitive's own vertical center to a shared
// horizontal line (the selection's overall vertical midpoint) - named after
// FidoCadJ's alignHorizontalCenterSelected, which centers things relative to
// a horizontal centerline (what other tools often call "align middle").
void MainWindow::clickAlignCenterHorizontalAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal topmost = std::numeric_limits<qreal>::max();
    qreal bottommost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected) {
        const QRectF rect = primitive->boundingRect();
        topmost = qMin(topmost, rect.top());
        bottommost = qMax(bottommost, rect.bottom());
    }
    const qreal verticalCenter = (topmost + bottommost) / 2.0;

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(0, verticalCenter - primitive->boundingRect().center().y()));
    moveSelectedPrimitives(deltas, tr("Align horizontal center"));
}

// Aligns every selected primitive's own horizontal center to a shared
// vertical line - FidoCadJ's alignVerticalCenterSelected ("align center" in
// more common CAD/DTP terminology).
void MainWindow::clickAlignCenterVerticalAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 2)
        return;

    qreal leftmost = std::numeric_limits<qreal>::max();
    qreal rightmost = std::numeric_limits<qreal>::lowest();
    for (GraphicsPrimitive *primitive : selected) {
        const QRectF rect = primitive->boundingRect();
        leftmost = qMin(leftmost, rect.left());
        rightmost = qMax(rightmost, rect.right());
    }
    const qreal horizontalCenter = (leftmost + rightmost) / 2.0;

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (GraphicsPrimitive *primitive : selected)
        deltas.insert(primitive, QPointF(horizontalCenter - primitive->boundingRect().center().x(), 0));
    moveSelectedPrimitives(deltas, tr("Align vertical center"));
}

// Distributes selected primitives evenly by their left edges, keeping the
// leftmost and rightmost primitives fixed - matching FidoCadJ's
// distributeHorizontallySelected() exactly, including its >=3 requirement
// (with only 2 selected there's nothing "in between" to redistribute).
void MainWindow::clickDistributeHorizontalAction()
{
    QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 3)
        return;

    std::sort(selected.begin(), selected.end(), [](GraphicsPrimitive *a, GraphicsPrimitive *b) {
        return a->boundingRect().left() < b->boundingRect().left();
    });

    const qreal leftmostX = selected.first()->boundingRect().left();
    const qreal rightmostX = selected.last()->boundingRect().left();
    const qreal spacing = (rightmostX - leftmostX) / (selected.size() - 1);

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (int i = 1; i < selected.size() - 1; ++i) {
        GraphicsPrimitive *primitive = selected.at(i);
        const qreal targetX = leftmostX + i * spacing;
        deltas.insert(primitive, QPointF(targetX - primitive->boundingRect().left(), 0));
    }
    moveSelectedPrimitives(deltas, tr("Distribute horizontally"));
}

void MainWindow::clickDistributeVerticalAction()
{
    QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.size() < 3)
        return;

    std::sort(selected.begin(), selected.end(), [](GraphicsPrimitive *a, GraphicsPrimitive *b) {
        return a->boundingRect().top() < b->boundingRect().top();
    });

    const qreal topmostY = selected.first()->boundingRect().top();
    const qreal bottommostY = selected.last()->boundingRect().top();
    const qreal spacing = (bottommostY - topmostY) / (selected.size() - 1);

    QHash<GraphicsPrimitive *, QPointF> deltas;
    for (int i = 1; i < selected.size() - 1; ++i) {
        GraphicsPrimitive *primitive = selected.at(i);
        const qreal targetY = topmostY + i * spacing;
        deltas.insert(primitive, QPointF(0, targetY - primitive->boundingRect().top()));
    }
    moveSelectedPrimitives(deltas, tr("Distribute vertically"));
}

// Copy/Cut put the selection on the system clipboard as FidoCadJ text
// (FidoCadWriter::writeSelection()) - matching the reference FidoCadJ editor,
// which copies as text rather than an app-private format, so a paste can also
// land in a plain text editor (and, conversely, so hand-written FCD snippets
// can be pasted straight into the drawing).
void MainWindow::clickCopyAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(selected));
}

QList<GraphicsPrimitive *> MainWindow::expandMacrosOneLevel(const QList<GraphicsPrimitive *> &source,
                                                              QList<GraphicsPrimitive *> &owned) const
{
    QList<GraphicsPrimitive *> result;
    for (GraphicsPrimitive *primitive : source) {
        if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib) {
            const QList<GraphicsPrimitive *> expanded =
                    static_cast<PrimitiveMacro *>(primitive)->convertToPrimitives(sheetScene);
            owned.append(expanded);
            result.append(expanded);
        } else {
            result.append(primitive);
        }
    }
    return result;
}

void MainWindow::clickCopySplitAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;

    QList<GraphicsPrimitive *> owned;
    const QList<GraphicsPrimitive *> expanded = expandMacrosOneLevel(selected, owned);
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(expanded));
    qDeleteAll(owned);
}

void MainWindow::clickCopyAsImageAction()
{
    if (sheetScene->itemsBoundingRect().isEmpty()) {
        QMessageBox::information(this, tr("Copy as image"), tr("The drawing is empty."));
        return;
    }

    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();

    // Selection highlight and handles are editor chrome, not drawing content
    // - same clear/restore pattern as the PNG/SVG/PDF exports.
    const QList<QGraphicsItem *> previousSelection = sheetScene->selectedItems();
    sheetScene->clearSelection();

    // With a selection, copy just those primitives: hide everything else for
    // this one render pass through the same primitive-owned visibility flag
    // the CLI's per-layer split export already uses (paint() early-returns
    // on it; it's independent of the QGraphicsItem visibility that layer
    // hiding drives). Without a selection, copy the whole drawing.
    QRectF sourceRect;
    if (!selected.isEmpty()) {
        for (GraphicsPrimitive *primitive : selected)
            sourceRect = sourceRect.united(primitive->sceneBoundingRect());
        for (GraphicsPrimitive *primitive : sheetScene->primitives())
            primitive->setVisible(selected.contains(primitive));
    } else {
        sourceRect = sheetScene->itemsBoundingRect();
    }

    // Same fixed pixels-per-drawing-unit ratio as the PNG export's default
    // scale factor - crisp enough to paste into documents without asking.
    const int scale = 4;
    const QSize targetSize = (sourceRect.size() * scale).toSize();
    QImage image(targetSize, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    sheetScene->render(&painter, QRectF(QPointF(0, 0), targetSize), sourceRect);
    painter.end();

    if (!selected.isEmpty()) {
        for (GraphicsPrimitive *primitive : sheetScene->primitives())
            primitive->setVisible(true);
    }

    QGuiApplication::clipboard()->setImage(image);

    for (QGraphicsItem *item : previousSelection)
        item->setSelected(true);
}

void MainWindow::clickCutAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(FidoCadWriter::writeSelection(selected));

    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = selected.size() > 1;
    if (multiple)
        undo->beginMacro(tr("Cut"));
    for (GraphicsPrimitive *primitive : selected)
        undo->push(new DeletePrimitiveCommand(sheetScene, primitive));
    if (multiple)
        undo->endMacro();
}

// Shared by Paste (parses clipboard text) and Duplicate (parses a freshly
// serialized copy of the current selection without touching the system
// clipboard at all - matching the visible result of the reference FidoCadJ
// editor's Duplicate, which is copySelected()+paste() under the hood, but
// without the side effect of clobbering whatever the user last copied).
void MainWindow::pasteFromText(const QString &text, const QString &undoLabel, bool inPlace)
{
    const QList<GraphicsPrimitive *> pasted = FidoCadReader::parse(text, sheetScene);
    if (pasted.isEmpty())
        return;

    // Offset by one grid step and select the result, matching the reference
    // FidoCadJ editor's paste (CopyPasteActions.paste() shifts by the current
    // grid step; PopUpMenu's Paste handler passes that grid step in) - so the
    // pasted copy is visibly distinct from what was copied and immediately
    // ready to drag into place. Paste in place skips the shift: the copy
    // lands exactly on the coordinates it was copied from.
    const QVariant stepVal = SettingsManager::getInstance().loadSetting("snap_step");
    const int step = stepVal.isValid() && stepVal.toInt() > 0 ? stepVal.toInt() : 10;
    const QPointF offset = inPlace ? QPointF(0, 0) : QPointF(step, step);

    const bool wasEmpty = sheetScene->primitives().isEmpty();

    sheetScene->clearSelection();
    QUndoStack *undo = sheetScene->undoStack();
    const bool multiple = pasted.size() > 1;
    if (multiple)
        undo->beginMacro(undoLabel);
    for (GraphicsPrimitive *primitive : pasted) {
        primitive->translateControlPoints(offset);
        // push() calls redo() synchronously, which is what actually adds the
        // primitive to the scene - only safe to select it afterwards.
        undo->push(new CreatePrimitiveCommand(sheetScene, primitive));
        primitive->setSelected(true);
    }
    if (multiple)
        undo->endMacro();

    // Pasting into an empty drawing: the content lands wherever its source
    // coordinates put it, which can easily be entirely off-screen - fit the
    // view to it so the result is immediately visible. A paste into an
    // existing drawing keeps the current view untouched.
    if (wasEmpty)
        activeView()->adjustView();
}

// Companion to pasteFromText(): the clipboard holds a bitmap (e.g. from
// "Copy as image", a screenshot tool, or another application) rather than
// FCD text. Mirrors PrimitivePlacementController::armImagePlacement()'s
// construction of a PrimitiveImage (same base64 storage, same 100-unit
// max-dimension scaling), but pushed straight onto the undo stack instead
// of going through interactive placement, since there's no "click to drop
// it" step here - Ctrl+V just puts it where armImagePlacement() seeds a
// freshly-armed image tool: the view's current center.
void MainWindow::pasteImageFromClipboard(const QImage &image)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    auto *primitiveImage = new PrimitiveImage();
    const QString base64Data = QString::fromLatin1(data.toBase64());
    primitiveImage->setImageData(QStringLiteral("png"), base64Data);

    constexpr qreal MaxDimension = 100.0;
    qreal width = MaxDimension, height = MaxDimension;
    if (image.width() > 0 && image.height() > 0) {
        const qreal scale = MaxDimension / qMax(image.width(), image.height());
        width = image.width() * scale;
        height = image.height() * scale;
    }
    const QPointF halfSize(width / 2.0, height / 2.0);
    const QPointF center = activeView()->mapToScene(activeView()->viewport()->rect().center());
    primitiveImage->setControlPoint(0, center - halfSize);
    primitiveImage->setControlPoint(1, center + halfSize);

    sheetScene->clearSelection();
    sheetScene->undoStack()->push(new CreatePrimitiveCommand(sheetScene, primitiveImage));
    primitiveImage->setSelected(true);
}

void MainWindow::clickPasteAction()
{
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard->mimeData()->hasImage()) {
        const QImage image = clipboard->image();
        if (!image.isNull()) {
            pasteImageFromClipboard(image);
            return;
        }
    }

    const QString text = clipboard->text();
    if (text.isEmpty())
        return;
    pasteFromText(text, tr("Paste"));
}

// Same as Paste, but without the one-grid-step shift - the copy lands on the
// exact coordinates it was copied from. An image clipboard still goes
// through pasteImageFromClipboard(): a bitmap has no source coordinates in
// the drawing, so "in place" can only mean the same view-centered drop.
void MainWindow::clickPasteInPlaceAction()
{
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard->mimeData()->hasImage()) {
        const QImage image = clipboard->image();
        if (!image.isNull()) {
            pasteImageFromClipboard(image);
            return;
        }
    }

    const QString text = clipboard->text();
    if (text.isEmpty())
        return;
    pasteFromText(text, tr("Paste in place"), true);
}

void MainWindow::clickDuplicateAction()
{
    const QList<GraphicsPrimitive *> selected = selectedPrimitivesInOrder();
    if (selected.isEmpty())
        return;
    pasteFromText(FidoCadWriter::writeSelection(selected), tr("Duplicate"));
}
