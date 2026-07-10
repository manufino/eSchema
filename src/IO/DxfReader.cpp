/*
 * DxfReader.cpp
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

#include "DxfReader.h"
#include "DxfCommon.h"
#include "Sheet.h"
#include "LayerList.h"
#include "Layer.h"
#include "PrimitiveLine.h"
#include "PrimitiveEllipse.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitivePolygon.h"
#include "PrimitiveConnection.h"
#include "PrimitivePcbTrack.h"
#include "PrimitiveText.h"

#include <QFile>
#include <QTextStream>
#include <QTransform>
#include <QHash>
#include <QSet>
#include <QRegularExpression>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace DxfReader {

namespace {

using namespace DxfCommon;

const int kMaxInsertDepth = 20;

// One code-0-delimited DXF record ("0 <TYPE>" plus every group up to, but
// not including, the next "0" group).
struct RawEntity {
    QString type;
    QVector<GroupPair> groups;
};

struct BlockDef {
    QPointF basePoint;
    QVector<RawEntity> entities;
};

// A polyline/legacy-POLYLINE vertex, with its optional per-vertex bulge
// (arc segment to the next vertex) and start/end width.
struct RawPoint {
    qreal x = 0, y = 0;
    qreal bulge = 0;
    qreal startWidth = 0, endWidth = 0;
};

struct ParsedPrimitive {
    GraphicsPrimitive *primitive;
    QString dxfLayer;
};

struct ParseContext {
    QList<ParsedPrimitive> primitives;
    QHash<QString, int> unsupportedCounts;
    int truncatedVertexCount = 0;
    int rotatedEllipseCount = 0;
    int insertDepthExceeded = 0;
    int missingBlockCount = 0;
};

QString findValue(const QVector<GroupPair> &groups, int code, const QString &fallback = QString())
{
    for (const GroupPair &g : groups)
        if (g.code == code)
            return g.value;
    return fallback;
}

QVector<QString> findValues(const QVector<GroupPair> &groups, int code)
{
    QVector<QString> values;
    for (const GroupPair &g : groups)
        if (g.code == code)
            values.append(g.value);
    return values;
}

// Splits pairs[start,end) into consecutive code-0-delimited records.
QVector<RawEntity> splitIntoEntities(const QVector<GroupPair> &pairs, int start, int end)
{
    QVector<RawEntity> result;
    int i = start;
    while (i < end) {
        if (pairs.at(i).code != 0) { ++i; continue; }
        RawEntity entity;
        entity.type = pairs.at(i).value;
        ++i;
        const int groupStart = i;
        while (i < end && pairs.at(i).code != 0)
            ++i;
        entity.groups = pairs.mid(groupStart, i - groupStart);
        result.append(entity);
    }
    return result;
}

// Body range [start,end) of the named top-level SECTION, or {-1,-1} if the
// file has none - not every DXF has TABLES/BLOCKS (a bare ENTITIES-only file
// is legal), so this is a normal, non-error outcome.
QPair<int, int> findSection(const QVector<GroupPair> &pairs, const QString &sectionName)
{
    for (int i = 0; i < pairs.size(); ++i) {
        if (pairs.at(i).code == 0 && pairs.at(i).value == QStringLiteral("SECTION")
                && i + 1 < pairs.size() && pairs.at(i + 1).code == 2
                && pairs.at(i + 1).value == sectionName) {
            const int bodyStart = i + 2;
            int j = bodyStart;
            while (j < pairs.size() && !(pairs.at(j).code == 0 && pairs.at(j).value == QStringLiteral("ENDSEC")))
                ++j;
            return {bodyStart, j};
        }
    }
    return {-1, -1};
}

QHash<QString, QColor> parseLayerTable(const QVector<GroupPair> &pairs)
{
    QHash<QString, QColor> table;
    const QPair<int, int> range = findSection(pairs, QStringLiteral("TABLES"));
    if (range.first < 0)
        return table;

    const QVector<RawEntity> records = splitIntoEntities(pairs, range.first, range.second);
    bool inLayerTable = false;
    for (const RawEntity &e : records) {
        if (e.type == QStringLiteral("TABLE")) {
            inLayerTable = findValue(e.groups, 2) == QStringLiteral("LAYER");
        } else if (e.type == QStringLiteral("ENDTAB")) {
            inLayerTable = false;
        } else if (inLayerTable && e.type == QStringLiteral("LAYER")) {
            const QString name = findValue(e.groups, 2, QStringLiteral("0"));
            const QString trueColor = findValue(e.groups, 420);
            QColor color;
            if (!trueColor.isEmpty()) {
                color = trueColorToQColor(trueColor.toLongLong());
            } else {
                bool ok = false;
                const int aci = findValue(e.groups, 62, QStringLiteral("7")).toInt(&ok);
                // A negative ACI just means "layer off" - the color itself
                // is still the absolute value.
                color = aciToColor(ok ? qAbs(aci) : 7);
            }
            table.insert(name, color);
        }
    }
    return table;
}

QHash<QString, BlockDef> parseBlocks(const QVector<GroupPair> &pairs)
{
    QHash<QString, BlockDef> blocks;
    const QPair<int, int> range = findSection(pairs, QStringLiteral("BLOCKS"));
    if (range.first < 0)
        return blocks;

    const QVector<RawEntity> records = splitIntoEntities(pairs, range.first, range.second);
    int i = 0;
    while (i < records.size()) {
        if (records.at(i).type != QStringLiteral("BLOCK")) { ++i; continue; }
        BlockDef def;
        const QString name = findValue(records.at(i).groups, 2);
        def.basePoint = QPointF(findValue(records.at(i).groups, 10, QStringLiteral("0")).toDouble(),
                                 findValue(records.at(i).groups, 20, QStringLiteral("0")).toDouble());
        ++i;
        while (i < records.size() && records.at(i).type != QStringLiteral("ENDBLK")) {
            def.entities.append(records.at(i));
            ++i;
        }
        if (i < records.size())
            ++i; // consume ENDBLK
        if (!name.isEmpty())
            blocks.insert(name, def);
    }
    return blocks;
}

// Standard bulge-to-arc conversion (Lee Mac's formula, as used by e.g.
// ezdxf.math.bulge_to_arc) - always returns a counter-clockwise start/end
// angle pair; the caller must swap point order for a negative bulge.
struct BulgeArc {
    QPointF center;
    qreal startAngleDeg;
    qreal endAngleDeg;
    qreal radius;
};

BulgeArc bulgeToArc(const QPointF &p1, const QPointF &p2, qreal bulge)
{
    const qreal dist = std::hypot(p2.x() - p1.x(), p2.y() - p1.y());
    const qreal r = dist * (1.0 + bulge * bulge) / 4.0 / bulge;
    const qreal segAngle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x());
    const qreal a = segAngle + (M_PI / 2.0 - std::atan(bulge) * 2.0);
    const QPointF c(p1.x() + std::cos(a) * r, p1.y() + std::sin(a) * r);
    const qreal angleToP1 = std::atan2(p1.y() - c.y(), p1.x() - c.x());
    const qreal angleToP2 = std::atan2(p2.y() - c.y(), p2.x() - c.x());

    BulgeArc result;
    result.center = c;
    result.radius = std::abs(r);
    if (bulge < 0) {
        result.startAngleDeg = qRadiansToDegrees(angleToP2);
        result.endAngleDeg = qRadiansToDegrees(angleToP1);
    } else {
        result.startAngleDeg = qRadiansToDegrees(angleToP1);
        result.endAngleDeg = qRadiansToDegrees(angleToP2);
    }
    return result;
}

// Expands raw vertices (with optional per-vertex bulge) into a dense point
// list, inserting sampled arc points for any bulge segment - same
// tessellation philosophy as ARC, so LWPOLYLINE/legacy POLYLINE and ARC
// entities degrade the same way. Caps at PrimitivePolygon::MaxVertices.
QVector<QPointF> expandPolylineVertices(const QVector<RawPoint> &verts, bool closed, ParseContext &ctx)
{
    QVector<QPointF> points;
    const int n = verts.size();
    for (int i = 0; i < n; ++i) {
        points.append(QPointF(verts.at(i).x, verts.at(i).y));
        const bool hasNext = (i + 1 < n) || closed;
        if (!hasNext)
            continue;
        const QPointF next(verts.at((i + 1) % n).x, verts.at((i + 1) % n).y);
        const qreal bulge = verts.at(i).bulge;
        if (!qFuzzyIsNull(bulge)) {
            const BulgeArc arc = bulgeToArc(points.last(), next, bulge);
            QVector<QPointF> arcPts = sampleArc(arc.center, arc.radius, arc.startAngleDeg, arc.endAngleDeg);
            if (bulge < 0)
                std::reverse(arcPts.begin(), arcPts.end());
            for (int k = 1; k < arcPts.size() - 1; ++k)
                points.append(arcPts.at(k));
        }
    }
    if (points.size() > PrimitivePolygon::MaxVertices) {
        points = points.mid(0, PrimitivePolygon::MaxVertices);
        ++ctx.truncatedVertexCount;
    }
    return points;
}

QVector<RawPoint> extractLwPolylineVertices(const QVector<GroupPair> &groups)
{
    QVector<RawPoint> points;
    bool haveX = false;
    for (const GroupPair &g : groups) {
        if (g.code == 10) {
            RawPoint p;
            p.x = g.value.toDouble();
            points.append(p);
            haveX = true;
        } else if (g.code == 20 && haveX) {
            points.last().y = g.value.toDouble();
        } else if (g.code == 40 && !points.isEmpty()) {
            points.last().startWidth = g.value.toDouble();
        } else if (g.code == 41 && !points.isEmpty()) {
            points.last().endWidth = g.value.toDouble();
        } else if (g.code == 42 && !points.isEmpty()) {
            points.last().bulge = g.value.toDouble();
        }
    }
    return points;
}

QString stripMTextFormatting(QString text)
{
    text.replace(QStringLiteral("\\P"), QStringLiteral(" "));
    text.replace(QStringLiteral("\\~"), QStringLiteral(" "));
    static const QRegularExpression formatting(QStringLiteral("\\\\[A-Za-z][^;\\\\{}]*;"));
    text.remove(formatting);
    text.remove(QLatin1Char('{'));
    text.remove(QLatin1Char('}'));
    text.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    return text;
}

// --- Per-entity-type builders ------------------------------------------------
// Each returns the primitive(s) it produced (0, 1, or several), still in
// local (pre-INSERT-transform) coordinates - the caller applies whatever
// transform is active and records the raw DXF layer name.

QList<GraphicsPrimitive *> buildLine(const RawEntity &e)
{
    const QPointF p1(findValue(e.groups, 10).toDouble(), findValue(e.groups, 20).toDouble());
    const QPointF p2(findValue(e.groups, 11).toDouble(), findValue(e.groups, 21).toDouble());
    if (QLineF(p1, p2).length() < 1e-9)
        return {};
    auto *p = new PrimitiveLine();
    p->setControlPoint(0, p1);
    p->setControlPoint(1, p2);
    return { p };
}

QList<GraphicsPrimitive *> buildCircle(const RawEntity &e)
{
    const QPointF center(findValue(e.groups, 10).toDouble(), findValue(e.groups, 20).toDouble());
    const qreal radius = findValue(e.groups, 40).toDouble();
    if (radius <= 0)
        return {};
    auto *p = new PrimitiveEllipse();
    p->setControlPoint(0, center - QPointF(radius, radius));
    p->setControlPoint(1, center + QPointF(radius, radius));
    return { p };
}

QList<GraphicsPrimitive *> buildArc(const RawEntity &e)
{
    const QPointF center(findValue(e.groups, 10).toDouble(), findValue(e.groups, 20).toDouble());
    const qreal radius = findValue(e.groups, 40).toDouble();
    const qreal startAngle = findValue(e.groups, 50).toDouble();
    const qreal endAngle = findValue(e.groups, 51).toDouble();
    if (radius <= 0)
        return {};
    const QVector<QPointF> pts = sampleArc(center, radius, startAngle, endAngle);
    if (pts.size() < 2)
        return {};
    auto *p = new PrimitiveComplexCurve();
    p->setClosed(false);
    for (const QPointF &pt : pts)
        p->appendVertex(pt);
    return { p };
}

// Axis-aligned bounding box of a (possibly rotated) ellipse - the standard
// formula halfW=sqrt((a*cosT)^2+(b*sinT)^2), halfH=sqrt((a*sinT)^2+(b*cosT)^2)
// reduces exactly to (a,b) when T is a multiple of 90 degrees, so this
// covers the axis-aligned case with no special-casing needed.
QList<GraphicsPrimitive *> buildEllipse(const RawEntity &e, bool &discardedRotation)
{
    discardedRotation = false;
    const QPointF center(findValue(e.groups, 10).toDouble(), findValue(e.groups, 20).toDouble());
    const qreal majorX = findValue(e.groups, 11).toDouble();
    const qreal majorY = findValue(e.groups, 21).toDouble();
    const qreal ratio = qBound(0.0001, findValue(e.groups, 40, QStringLiteral("1")).toDouble(), 1.0);
    const qreal a = std::hypot(majorX, majorY);
    if (a <= 0)
        return {};
    const qreal b = a * ratio;
    const qreal theta = std::atan2(majorY, majorX);
    const qreal cosT = std::cos(theta), sinT = std::sin(theta);
    const qreal halfWidth = std::sqrt(a * cosT * a * cosT + b * sinT * b * sinT);
    const qreal halfHeight = std::sqrt(a * sinT * a * sinT + b * cosT * b * cosT);
    const qreal thetaMod = std::fmod(std::abs(theta), M_PI / 2.0);
    discardedRotation = thetaMod > 0.01 && (M_PI / 2.0 - thetaMod) > 0.01;

    auto *p = new PrimitiveEllipse();
    p->setControlPoint(0, center - QPointF(halfWidth, halfHeight));
    p->setControlPoint(1, center + QPointF(halfWidth, halfHeight));
    return { p };
}

QList<GraphicsPrimitive *> buildFromVertices(const QVector<RawPoint> &verts, bool closed, ParseContext &ctx)
{
    // A widthed, open, exactly-2-vertex polyline is the structurally
    // distinctive signature written by DxfWriter for PrimitivePcbTrack -
    // recognized unconditionally, since a plain LINE never carries width.
    if (!closed && verts.size() == 2) {
        const qreal width = std::max({ verts.at(0).startWidth, verts.at(0).endWidth,
                                        verts.at(1).startWidth, verts.at(1).endWidth });
        if (width > 0) {
            auto *p = new PrimitivePcbTrack();
            p->setControlPoint(0, QPointF(verts.at(0).x, verts.at(0).y));
            p->setControlPoint(1, QPointF(verts.at(1).x, verts.at(1).y));
            p->setWidth(width);
            return { p };
        }
    }

    const QVector<QPointF> points = expandPolylineVertices(verts, closed, ctx);
    if (points.size() < 2)
        return {};

    if (closed) {
        auto *p = new PrimitivePolygon();
        for (const QPointF &pt : points)
            p->appendVertex(pt);
        return { p };
    }

    // Open polyline: a chain of straight PrimitiveLine segments, not a
    // PrimitiveComplexCurve - the segments are genuinely straight by
    // definition, and reinterpreting them as spline control points would
    // visibly round off sharp corners that aren't actually curved.
    QList<GraphicsPrimitive *> lines;
    for (int i = 0; i + 1 < points.size(); ++i) {
        auto *p = new PrimitiveLine();
        p->setControlPoint(0, points.at(i));
        p->setControlPoint(1, points.at(i + 1));
        lines.append(p);
    }
    return lines;
}

QList<GraphicsPrimitive *> buildLwPolyline(const RawEntity &e, ParseContext &ctx)
{
    const bool closed = (findValue(e.groups, 70, QStringLiteral("0")).toInt() & 1) != 0;
    const QVector<RawPoint> verts = extractLwPolylineVertices(e.groups);
    if (verts.size() < 2)
        return {};
    return buildFromVertices(verts, closed, ctx);
}

QList<GraphicsPrimitive *> buildSpline(const RawEntity &e, ParseContext &ctx)
{
    QVector<QString> xs = findValues(e.groups, 11);
    QVector<QString> ys = findValues(e.groups, 21);
    if (xs.isEmpty()) {
        // No fit points - fall back to control points.
        xs = findValues(e.groups, 10);
        ys = findValues(e.groups, 20);
    }
    const int count = qMin(xs.size(), ys.size());
    if (count < 2)
        return {};

    const bool closed = (findValue(e.groups, 70, QStringLiteral("0")).toInt() & 1) != 0;
    int vertexCount = qMin(count, int(PrimitiveComplexCurve::MaxVertices));
    if (vertexCount < count)
        ++ctx.truncatedVertexCount;

    auto *p = new PrimitiveComplexCurve();
    p->setClosed(closed);
    for (int i = 0; i < vertexCount; ++i)
        p->appendVertex(QPointF(xs.at(i).toDouble(), ys.at(i).toDouble()));
    return { p };
}

QList<GraphicsPrimitive *> buildText(const RawEntity &e, bool isMText)
{
    const QPointF pos(findValue(e.groups, 10).toDouble(), findValue(e.groups, 20).toDouble());
    qreal height = findValue(e.groups, 40, QStringLiteral("4")).toDouble();
    if (height <= 0)
        height = 4;
    const qreal rotation = findValue(e.groups, 50, QStringLiteral("0")).toDouble();

    QString text;
    if (isMText) {
        for (const QString &chunk : findValues(e.groups, 3))
            text += chunk;
        text += findValue(e.groups, 1);
        text = stripMTextFormatting(text);
    } else {
        text = findValue(e.groups, 1);
    }
    if (text.trimmed().isEmpty())
        return {};

    auto *p = new PrimitiveText();
    p->setControlPoint(0, pos);
    p->setSize(qMax(1, int(qRound(height))), qMax(1, int(qRound(height * 0.75))));
    p->setOrientationDeg(int(qRound(rotation)));
    p->setText(text);
    return { p };
}

QList<GraphicsPrimitive *> buildPoint(const RawEntity &e)
{
    auto *p = new PrimitiveConnection();
    p->setControlPoint(0, QPointF(findValue(e.groups, 10).toDouble(), findValue(e.groups, 20).toDouble()));
    return { p };
}

// Applies the accumulated INSERT transform to every control point, plus (for
// PrimitiveText, whose rotation/size are scalar fields rather than derivable
// from a single anchor point) the accumulated rotation/scale directly.
void applyInsertTransform(GraphicsPrimitive *primitive, const QTransform &transform,
                           qreal accumRotationDeg, qreal accumScale)
{
    for (int i = 0; i < primitive->controlPointCount(); ++i)
        primitive->setControlPoint(i, transform.map(primitive->controlPoint(i)));

    if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
        auto *text = static_cast<PrimitiveText *>(primitive);
        text->setOrientationDeg(int(qRound(text->orientationDeg() + accumRotationDeg)));
        text->setSize(qMax(1, int(qRound(text->sizeY() * accumScale))),
                      qMax(1, int(qRound(text->sizeX() * accumScale))));
    }
}

void processRecords(const QVector<RawEntity> &records, const QHash<QString, BlockDef> &blocks,
                     const QTransform &transform, qreal accumRotationDeg, qreal accumScale,
                     int depth, ParseContext &ctx)
{
    int i = 0;
    while (i < records.size()) {
        const RawEntity &e = records.at(i);
        const QString &type = e.type;

        if (type == QStringLiteral("POLYLINE")) {
            // Legacy POLYLINE/VERTEX/SEQEND cluster - gather every following
            // VERTEX record until SEQEND, a structurally different shape
            // than LWPOLYLINE's single flat record.
            const QString layerName = findValue(e.groups, 8, QStringLiteral("0"));
            const bool closed = (findValue(e.groups, 70, QStringLiteral("0")).toInt() & 1) != 0;
            QVector<RawPoint> verts;
            ++i;
            while (i < records.size() && records.at(i).type == QStringLiteral("VERTEX")) {
                RawPoint rp;
                rp.x = findValue(records.at(i).groups, 10).toDouble();
                rp.y = findValue(records.at(i).groups, 20).toDouble();
                rp.bulge = findValue(records.at(i).groups, 42, QStringLiteral("0")).toDouble();
                verts.append(rp);
                ++i;
            }
            if (i < records.size() && records.at(i).type == QStringLiteral("SEQEND"))
                ++i;
            if (verts.size() >= 2) {
                for (GraphicsPrimitive *p : buildFromVertices(verts, closed, ctx)) {
                    applyInsertTransform(p, transform, accumRotationDeg, accumScale);
                    ctx.primitives.append({ p, layerName });
                }
            }
            continue;
        }

        if (type == QStringLiteral("INSERT")) {
            const QString blockName = findValue(e.groups, 2);
            if (depth >= kMaxInsertDepth) {
                ++ctx.insertDepthExceeded;
            } else if (!blocks.contains(blockName)) {
                ++ctx.missingBlockCount;
            } else {
                const BlockDef &block = blocks.value(blockName);
                const QPointF insertPoint(findValue(e.groups, 10, QStringLiteral("0")).toDouble(),
                                           findValue(e.groups, 20, QStringLiteral("0")).toDouble());
                qreal xScale = findValue(e.groups, 41, QStringLiteral("1")).toDouble();
                qreal yScale = findValue(e.groups, 42, QStringLiteral("1")).toDouble();
                if (qFuzzyIsNull(xScale)) xScale = 1.0;
                if (qFuzzyIsNull(yScale)) yScale = 1.0;
                const qreal rotationDeg = findValue(e.groups, 50, QStringLiteral("0")).toDouble();

                // Maps the block's own local space to the space `transform`
                // already maps into - block-local -> subtract base point ->
                // scale -> rotate -> translate to the insertion point.
                QTransform local;
                local.translate(insertPoint.x(), insertPoint.y());
                local.rotate(rotationDeg);
                local.scale(xScale, yScale);
                local.translate(-block.basePoint.x(), -block.basePoint.y());
                const QTransform combined = local * transform;

                processRecords(block.entities, blocks, combined, accumRotationDeg + rotationDeg,
                                accumScale * (std::abs(xScale) + std::abs(yScale)) / 2.0, depth + 1, ctx);
            }
            ++i;
            continue;
        }

        QList<GraphicsPrimitive *> built;
        if (type == QStringLiteral("LINE")) {
            built = buildLine(e);
        } else if (type == QStringLiteral("CIRCLE")) {
            built = buildCircle(e);
        } else if (type == QStringLiteral("ARC")) {
            built = buildArc(e);
        } else if (type == QStringLiteral("ELLIPSE")) {
            bool discardedRotation = false;
            built = buildEllipse(e, discardedRotation);
            if (discardedRotation)
                ++ctx.rotatedEllipseCount;
        } else if (type == QStringLiteral("LWPOLYLINE")) {
            built = buildLwPolyline(e, ctx);
        } else if (type == QStringLiteral("SPLINE")) {
            built = buildSpline(e, ctx);
        } else if (type == QStringLiteral("TEXT")) {
            built = buildText(e, false);
        } else if (type == QStringLiteral("MTEXT")) {
            built = buildText(e, true);
        } else if (type == QStringLiteral("POINT")) {
            built = buildPoint(e);
        } else if (type != QStringLiteral("SEQEND") && type != QStringLiteral("VERTEX")) {
            // Any other entity type has no DXF<->eSchema mapping (HATCH,
            // DIMENSION, LEADER/MLEADER, SOLID/3DFACE, XLINE/RAY,
            // TOLERANCE, WIPEOUT, ...) - counted for the post-import summary.
            ++ctx.unsupportedCounts[type];
        }

        const QString layerName = findValue(e.groups, 8, QStringLiteral("0"));
        for (GraphicsPrimitive *p : built) {
            applyInsertTransform(p, transform, accumRotationDeg, accumScale);
            ctx.primitives.append({ p, layerName });
        }
        ++i;
    }
}

// Renames/recolors the existing global 16-slot LayerList to mirror the
// distinct DXF layers actually used by a kept (non-skipped) entity, in
// first-encounter order - reusing any eSchema layer whose name already
// matches case-insensitively before claiming a fresh slot. Layers beyond
// the 16 available slots all fall back onto the last-claimed one.
QHash<QString, Layer *> remapLayers(const QStringList &usedLayerNamesInOrder,
                                     const QHash<QString, QColor> &layerColors, QStringList *warnings)
{
    QHash<QString, Layer *> mapping;
    QList<Layer *> *layers = LayerList::getInstance().getList();
    if (!layers || layers->isEmpty())
        return mapping;

    const int slotCount = layers->size();
    QSet<int> claimedSlots;

    for (const QString &name : usedLayerNamesInOrder) {
        for (int i = 0; i < slotCount; ++i) {
            if (claimedSlots.contains(i))
                continue;
            if (layers->at(i)->name().compare(name, Qt::CaseInsensitive) == 0) {
                mapping.insert(name, layers->at(i));
                claimedSlots.insert(i);
                break;
            }
        }
    }

    int overflow = 0;
    int nextSlot = 0;
    Layer *lastClaimed = layers->at(0);
    for (const QString &name : usedLayerNamesInOrder) {
        if (mapping.contains(name)) {
            lastClaimed = mapping.value(name);
            continue;
        }
        while (nextSlot < slotCount && claimedSlots.contains(nextSlot))
            ++nextSlot;
        if (nextSlot < slotCount) {
            Layer *layer = layers->at(nextSlot);
            layer->setName(name);
            layer->setColor(layerColors.value(name, QColor(Qt::white)));
            mapping.insert(name, layer);
            claimedSlots.insert(nextSlot);
            lastClaimed = layer;
            ++nextSlot;
        } else {
            mapping.insert(name, lastClaimed);
            ++overflow;
        }
    }

    if (overflow > 0 && warnings) {
        warnings->append(QObject::tr("%1 layer del DXF oltre il limite di 16 sono stati uniti "
                                      "nell'ultimo layer disponibile.").arg(overflow));
    }

    LayerList::getInstance().update();
    return mapping;
}

} // namespace

void read(const QString &text, Sheet *sheet, QStringList *warnings)
{
    sheet->clearPrimitives();

    const QVector<GroupPair> pairs = tokenizePairs(text);
    const QHash<QString, QColor> layerColors = parseLayerTable(pairs);
    const QHash<QString, BlockDef> blocks = parseBlocks(pairs);

    ParseContext ctx;
    const QPair<int, int> entitiesRange = findSection(pairs, QStringLiteral("ENTITIES"));
    if (entitiesRange.first >= 0) {
        const QVector<RawEntity> records = splitIntoEntities(pairs, entitiesRange.first, entitiesRange.second);
        processRecords(records, blocks, QTransform(), 0.0, 1.0, 0, ctx);
    }

    QStringList usedLayerNames;
    QSet<QString> seenLayerNames;
    for (const ParsedPrimitive &pp : ctx.primitives) {
        if (!seenLayerNames.contains(pp.dxfLayer)) {
            seenLayerNames.insert(pp.dxfLayer);
            usedLayerNames.append(pp.dxfLayer);
        }
    }
    const QHash<QString, Layer *> layerMapping = remapLayers(usedLayerNames, layerColors, warnings);

    for (const ParsedPrimitive &pp : ctx.primitives) {
        if (pp.primitive->isDegenerate()) {
            delete pp.primitive;
            continue;
        }
        pp.primitive->setLayer(layerMapping.value(pp.dxfLayer, LayerList::getInstance().getMaster()));
        sheet->addPrimitive(pp.primitive);
    }

    if (!warnings)
        return;

    for (auto it = ctx.unsupportedCounts.constBegin(); it != ctx.unsupportedCounts.constEnd(); ++it) {
        warnings->append(QObject::tr("%1 entità %2 non supportate sono state ignorate.")
                                  .arg(it.value()).arg(it.key()));
    }
    if (ctx.truncatedVertexCount > 0) {
        warnings->append(QObject::tr("%1 elementi con più di %2 vertici sono stati troncati.")
                                  .arg(ctx.truncatedVertexCount).arg(PrimitivePolygon::MaxVertices));
    }
    if (ctx.rotatedEllipseCount > 0) {
        warnings->append(QObject::tr("%1 ellissi ruotate sono state importate come non ruotate "
                                      "(bounding box).").arg(ctx.rotatedEllipseCount));
    }
    if (ctx.missingBlockCount > 0) {
        warnings->append(QObject::tr("%1 riferimenti INSERT a blocchi non trovati sono stati ignorati.")
                                  .arg(ctx.missingBlockCount));
    }
    if (ctx.insertDepthExceeded > 0) {
        warnings->append(QObject::tr("%1 blocchi annidati oltre il limite di profondità sono stati ignorati.")
                                  .arg(ctx.insertDepthExceeded));
    }
    if (entitiesRange.first < 0)
        warnings->append(QObject::tr("Il file non contiene una sezione ENTITIES."));
}

bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage, QStringList *warnings)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    read(stream.readAll(), sheet, warnings);
    return true;
}

} // namespace DxfReader
