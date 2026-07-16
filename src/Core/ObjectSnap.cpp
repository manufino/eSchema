/*
 * ObjectSnap.cpp
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

#include "ObjectSnap.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include "SettingsManager.h"
#include <QLineF>
#include <QRectF>
#include <QVector>

namespace {

qreal distanceSq(const QPointF &a, const QPointF &b)
{
    const QPointF diff = a - b;
    return QPointF::dotProduct(diff, diff);
}

// Squared distance from `p` to the segment [a, b].
qreal pointToSegmentDistanceSq(const QPointF &p, const QPointF &a, const QPointF &b)
{
    const QPointF ab = b - a;
    const qreal lengthSq = QPointF::dotProduct(ab, ab);
    qreal t = lengthSq > 0 ? QPointF::dotProduct(p - a, ab) / lengthSq : 0.0;
    t = qBound<qreal>(0.0, t, 1.0);
    return distanceSq(p, a + ab * t);
}

bool snapKindEnabled(const char *key)
{
    const QVariant value = SettingsManager::getInstance().loadSetting(key);
    return value.isValid() ? value.toBool() : true;
}

}

std::optional<QPointF> ObjectSnap::snapPoint(const Sheet *sheet, const QPointF &scenePos,
                                             qreal radius,
                                             const QList<const GraphicsPrimitive *> &excluded)
{
    // Each candidate kind can be turned off individually from the Options
    // dialog's Snap page.
    const bool wantEndpoints = snapKindEnabled("object_snap_endpoints");
    const bool wantMidpoints = snapKindEnabled("object_snap_midpoints");
    const bool wantCenters = snapKindEnabled("object_snap_centers");
    const bool wantIntersections = snapKindEnabled("object_snap_intersections");

    const qreal radiusSq = radius * radius;
    std::optional<QPointF> best;
    qreal bestSq = radiusSq;
    auto consider = [&](const QPointF &candidate) {
        const qreal dsq = distanceSq(candidate, scenePos);
        if (dsq <= bestSq) {
            bestSq = dsq;
            best = candidate;
        }
    };
    auto considerEndpoint = [&](const QPointF &candidate) {
        if (wantEndpoints)
            consider(candidate);
    };
    auto considerCenter = [&](const QPointF &candidate) {
        if (wantCenters)
            consider(candidate);
    };

    // Straight segments passing near the cursor, collected while walking the
    // primitives - any pairwise crossing among them is an intersection
    // candidate. Only segments within `radius` of the cursor can contribute
    // one (the intersection must itself land within `radius`), which keeps
    // the pairwise loop tiny.
    QVector<QLineF> nearbySegments;
    auto considerSegment = [&](const QPointF &a, const QPointF &b) {
        if (wantMidpoints)
            consider((a + b) / 2.0);
        if (wantIntersections && pointToSegmentDistanceSq(scenePos, a, b) <= radiusSq)
            nearbySegments.append(QLineF(a, b));
    };

    for (GraphicsPrimitive *primitive : sheet->primitives()) {
        if (excluded.contains(primitive) || !primitive->isOnCanvas())
            continue;
        // Quick reject on the inflated bounding box, so a big drawing costs
        // little per mouse move.
        const QRectF reach = primitive->sceneBoundingRect().adjusted(-radius, -radius, radius, radius);
        if (!reach.contains(scenePos))
            continue;

        const int count = primitive->controlPointCount();
        switch (primitive->getPrimitiveType()) {
        case GraphicsPrimitive::Bezier:
            // Only the endpoints actually lie on the curve - the two inner
            // control points are off-curve guides, meaningless as targets.
            if (count >= 2) {
                considerEndpoint(primitive->controlPoint(0));
                considerEndpoint(primitive->controlPoint(count - 1));
            }
            break;
        case GraphicsPrimitive::Line:
        case GraphicsPrimitive::PcbTrack:
            if (count >= 2) {
                considerEndpoint(primitive->controlPoint(0));
                considerEndpoint(primitive->controlPoint(1));
                considerSegment(primitive->controlPoint(0), primitive->controlPoint(1));
            }
            break;
        case GraphicsPrimitive::Polyline: {
            for (int i = 0; i < count; ++i)
                considerEndpoint(primitive->controlPoint(i));
            for (int i = 0; i < count; ++i) // closed ring - wrap the last edge
                considerSegment(primitive->controlPoint(i), primitive->controlPoint((i + 1) % count));
            break;
        }
        case GraphicsPrimitive::Rectangle: {
            const QRectF rect = QRectF(primitive->controlPoint(0), primitive->controlPoint(1)).normalized();
            considerCenter(rect.center());
            considerSegment(rect.topLeft(), rect.topRight());
            considerSegment(rect.topRight(), rect.bottomRight());
            considerSegment(rect.bottomRight(), rect.bottomLeft());
            considerSegment(rect.bottomLeft(), rect.topLeft());
            considerEndpoint(rect.topLeft());
            considerEndpoint(rect.topRight());
            considerEndpoint(rect.bottomLeft());
            considerEndpoint(rect.bottomRight());
            break;
        }
        case GraphicsPrimitive::Ellipse: {
            const QRectF rect = QRectF(primitive->controlPoint(0), primitive->controlPoint(1)).normalized();
            considerCenter(rect.center());
            // The four quadrant points - where the ellipse meets its axes.
            considerEndpoint(QPointF(rect.center().x(), rect.top()));
            considerEndpoint(QPointF(rect.center().x(), rect.bottom()));
            considerEndpoint(QPointF(rect.left(), rect.center().y()));
            considerEndpoint(QPointF(rect.right(), rect.center().y()));
            break;
        }
        default:
            // Spline vertices, macro/text/pad/connection anchors, image
            // corners: the control points themselves are the natural targets.
            for (int i = 0; i < count; ++i)
                considerEndpoint(primitive->controlPoint(i));
            break;
        }
    }

    // Guides take part in intersection snapping too: where two guides cross,
    // and where a guide crosses a primitive's segment, is exactly the kind
    // of construction point guides get dragged out for. Each guide passing
    // within the radius contributes a short stretch of its (infinite) line,
    // centered on the cursor and long enough to cover any crossing that
    // could still land within the capture radius - so the existing pairwise
    // loop below picks the crossings up like any other intersection.
    const QVariant snapGuides = SettingsManager::getInstance().loadSetting("snap_to_guides");
    if (wantIntersections && (!snapGuides.isValid() || snapGuides.toBool())) {
        const qreal span = radius * 2.0;
        const QList<Sheet::Guide> &guides = sheet->guides();
        for (const Sheet::Guide &guide : guides) {
            if (guide.orientation == Qt::Vertical) {
                if (qAbs(scenePos.x() - guide.position) <= radius)
                    nearbySegments.append(QLineF(guide.position, scenePos.y() - span,
                                                 guide.position, scenePos.y() + span));
            } else {
                if (qAbs(scenePos.y() - guide.position) <= radius)
                    nearbySegments.append(QLineF(scenePos.x() - span, guide.position,
                                                 scenePos.x() + span, guide.position));
            }
        }
    }

    for (int i = 0; i < nearbySegments.size(); ++i) {
        for (int j = i + 1; j < nearbySegments.size(); ++j) {
            QPointF crossing;
            if (nearbySegments.at(i).intersects(nearbySegments.at(j), &crossing)
                    == QLineF::BoundedIntersection)
                consider(crossing);
        }
    }

    return best;
}
