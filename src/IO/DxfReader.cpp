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

#include "libdxfrw.h"
#include "drw_interface.h"

#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTransform>
#include <QHash>
#include <QSet>
#include <QLineF>
#include <QRegularExpression>
#include <QtMath>
#include <cmath>
#include <algorithm>

namespace DxfReader {

namespace {

using namespace DxfCommon;

const int kMaxInsertDepth = 20; // safety net only - see addInsert(), flattening here is never truly recursive

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

// A block's entities are stored already fully resolved to the block's own
// local coordinate space (any nested INSERT inside this block's own body
// was flattened - see addInsert() - while this block was being read), so
// instantiating this block later is always a single transform pass, never
// a recursive one.
struct BlockDef {
    QPointF basePoint;
    QList<ParsedPrimitive> entities;
};

struct ParseContext {
    QHash<QString, int> unsupportedCounts;
    int truncatedVertexCount = 0;
    int rotatedEllipseCount = 0;
    int missingBlockCount = 0;
};

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

// Deep copy of a primitive this reader can produce (only the 7 types below -
// buildFromVertices()/addX() never construct anything else). Used to
// instantiate a block's stored entities at each of its INSERT placements,
// since the same block can be placed more than once and each placement
// needs its own independent primitive.
GraphicsPrimitive *clonePrimitive(GraphicsPrimitive *src)
{
    GraphicsPrimitive *dst = nullptr;
    switch (src->getPrimitiveType()) {
    case GraphicsPrimitive::Line:
        dst = new PrimitiveLine();
        break;
    case GraphicsPrimitive::Ellipse:
        dst = new PrimitiveEllipse();
        break;
    case GraphicsPrimitive::Spline: {
        auto *c = new PrimitiveComplexCurve();
        c->setClosed(static_cast<PrimitiveComplexCurve *>(src)->isClosed());
        for (int i = 0; i < src->controlPointCount(); ++i)
            c->appendVertex(src->controlPoint(i));
        return c;
    }
    case GraphicsPrimitive::Polyline: {
        auto *c = new PrimitivePolygon();
        for (int i = 0; i < src->controlPointCount(); ++i)
            c->appendVertex(src->controlPoint(i));
        return c;
    }
    case GraphicsPrimitive::Connection:
        dst = new PrimitiveConnection();
        break;
    case GraphicsPrimitive::PcbTrack:
        dst = new PrimitivePcbTrack();
        static_cast<PrimitivePcbTrack *>(dst)->setWidth(static_cast<PrimitivePcbTrack *>(src)->width());
        break;
    case GraphicsPrimitive::Text: {
        auto *s = static_cast<PrimitiveText *>(src);
        auto *t = new PrimitiveText();
        t->setText(s->text());
        t->setSize(s->sizeY(), s->sizeX());
        t->setOrientationDeg(s->orientationDeg());
        dst = t;
        break;
    }
    default:
        return nullptr; // never produced by this reader
    }
    for (int i = 0; i < src->controlPointCount(); ++i)
        dst->setControlPoint(i, src->controlPoint(i));
    return dst;
}

// Applies one INSERT's transform to a primitive's geometry, plus (for
// PrimitiveText, whose rotation/size are scalar fields rather than
// derivable from a single anchor point) this step's own rotation/scale
// directly - correct without needing an accumulated total threaded through
// calls, since each block's entities are already fully resolved to that
// block's own local space by the time an outer INSERT places it (see
// BlockDef's comment).
void applyInsertTransform(GraphicsPrimitive *primitive, const QTransform &transform,
                           qreal rotationDeg, qreal scale)
{
    for (int i = 0; i < primitive->controlPointCount(); ++i)
        primitive->setControlPoint(i, transform.map(primitive->controlPoint(i)));

    if (primitive->getPrimitiveType() == GraphicsPrimitive::Text) {
        auto *text = static_cast<PrimitiveText *>(primitive);
        text->setOrientationDeg(int(qRound(text->orientationDeg() + rotationDeg)));
        text->setSize(qMax(1, int(qRound(text->sizeY() * scale))),
                      qMax(1, int(qRound(text->sizeX() * scale))));
    }
}

// Implements DRW_Interface for reading: the addX() callbacks fire for both
// top-level ENTITIES-section entities and (while inside an addBlock()/
// endBlock() pair) a block definition's own body - emit_() routes to
// whichever is currently active. Every entity type this app has no mapping
// for (HATCH, DIMENSION*, LEADER, SOLID/3DFACE, ...) is a one-line counting
// no-op. The write-side DRW_Interface methods are never called on the read
// path but are still pure virtual and must be overridden.
class ImportInterface : public DRW_Interface {
public:
    QList<ParsedPrimitive> topLevel;
    QHash<QString, QColor> layerColors;
    QHash<QString, BlockDef> blocks;
    ParseContext ctx;

    bool inBlock = false;
    QString currentBlockName;
    int insertDepth = 0;

    void emitPrimitive(GraphicsPrimitive *p, const QString &layerName)
    {
        if (!p)
            return;
        if (inBlock)
            blocks[currentBlockName].entities.append({ p, layerName });
        else
            topLevel.append({ p, layerName });
    }

    void buildText(const DRW_Text &data, bool isMText)
    {
        QString text = QString::fromStdString(data.text);
        if (isMText)
            text = stripMTextFormatting(text);
        if (text.trimmed().isEmpty())
            return;
        const qreal height = data.height > 0 ? data.height : 4.0;
        auto *p = new PrimitiveText();
        p->setControlPoint(0, QPointF(data.basePoint.x, data.basePoint.y));
        p->setSize(qMax(1, int(qRound(height))), qMax(1, int(qRound(height * 0.75))));
        // DXF TEXT/MTEXT rotation (code 50) is in degrees, unlike ARC/ELLIPSE
        // angles which libdxfrw hands back in radians.
        p->setOrientationDeg(int(qRound(data.angle)));
        p->setText(text);
        emitPrimitive(p, QString::fromStdString(data.layer));
    }

    // --- read-side callbacks actually used -----------------------------------
    void addLayer(const DRW_Layer &data) override
    {
        const QString name = QString::fromStdString(data.name);
        const QColor color = data.color24 >= 0 ? trueColorToQColor(data.color24)
                                                : aciToColor(qAbs(data.color));
        layerColors.insert(name, color);
    }

    void addBlock(const DRW_Block &data) override
    {
        inBlock = true;
        currentBlockName = QString::fromStdString(data.name);
        blocks[currentBlockName].basePoint = QPointF(data.basePoint.x, data.basePoint.y);
    }
    void setBlock(const int) override {} // DWG-only, never fires on the ASCII DXF path
    void endBlock() override { inBlock = false; currentBlockName.clear(); }

    void addPoint(const DRW_Point &data) override
    {
        auto *p = new PrimitiveConnection();
        p->setControlPoint(0, QPointF(data.basePoint.x, data.basePoint.y));
        emitPrimitive(p, QString::fromStdString(data.layer));
    }

    void addLine(const DRW_Line &data) override
    {
        const QPointF p1(data.basePoint.x, data.basePoint.y);
        const QPointF p2(data.secPoint.x, data.secPoint.y);
        if (QLineF(p1, p2).length() < 1e-9)
            return;
        auto *p = new PrimitiveLine();
        p->setControlPoint(0, p1);
        p->setControlPoint(1, p2);
        emitPrimitive(p, QString::fromStdString(data.layer));
    }

    void addArc(const DRW_Arc &data) override
    {
        if (data.radious <= 0)
            return;
        const QPointF center(data.basePoint.x, data.basePoint.y);
        const qreal startDeg = qRadiansToDegrees(data.staangle);
        const qreal endDeg = qRadiansToDegrees(data.endangle);
        const QVector<QPointF> pts = sampleArc(center, data.radious, startDeg, endDeg);
        if (pts.size() < 2)
            return;
        auto *p = new PrimitiveComplexCurve();
        p->setClosed(false);
        for (const QPointF &pt : pts)
            p->appendVertex(pt);
        emitPrimitive(p, QString::fromStdString(data.layer));
    }

    void addCircle(const DRW_Circle &data) override
    {
        if (data.radious <= 0)
            return;
        const QPointF center(data.basePoint.x, data.basePoint.y);
        auto *p = new PrimitiveEllipse();
        p->setControlPoint(0, center - QPointF(data.radious, data.radious));
        p->setControlPoint(1, center + QPointF(data.radious, data.radious));
        emitPrimitive(p, QString::fromStdString(data.layer));
    }

    // Axis-aligned bounding box of a (possibly rotated) ellipse - reduces
    // exactly to (a,b) when the rotation is a multiple of 90 degrees, so
    // this covers the axis-aligned case with no special-casing needed.
    void addEllipse(const DRW_Ellipse &data) override
    {
        const QPointF center(data.basePoint.x, data.basePoint.y);
        const qreal majorX = data.secPoint.x, majorY = data.secPoint.y;
        const qreal ratio = qBound(0.0001, data.ratio, 1.0);
        const qreal a = std::hypot(majorX, majorY);
        if (a <= 0)
            return;
        const qreal b = a * ratio;
        const qreal theta = std::atan2(majorY, majorX);
        const qreal cosT = std::cos(theta), sinT = std::sin(theta);
        const qreal halfWidth = std::sqrt(a * cosT * a * cosT + b * sinT * b * sinT);
        const qreal halfHeight = std::sqrt(a * sinT * a * sinT + b * cosT * b * cosT);
        const qreal thetaMod = std::fmod(std::abs(theta), M_PI / 2.0);
        if (thetaMod > 0.01 && (M_PI / 2.0 - thetaMod) > 0.01)
            ++ctx.rotatedEllipseCount;

        auto *p = new PrimitiveEllipse();
        p->setControlPoint(0, center - QPointF(halfWidth, halfHeight));
        p->setControlPoint(1, center + QPointF(halfWidth, halfHeight));
        emitPrimitive(p, QString::fromStdString(data.layer));
    }

    void addLWPolyline(const DRW_LWPolyline &data) override
    {
        const bool closed = (data.flags & 1) != 0;
        QVector<RawPoint> verts;
        for (const auto &v : data.vertlist)
            verts.append({ v->x, v->y, v->bulge, v->stawidth, v->endwidth });
        if (verts.size() < 2)
            return;
        const QString layer = QString::fromStdString(data.layer);
        for (GraphicsPrimitive *p : buildFromVertices(verts, closed, ctx))
            emitPrimitive(p, layer);
    }

    void addPolyline(const DRW_Polyline &data) override
    {
        const bool closed = (data.flags & 1) != 0;
        QVector<RawPoint> verts;
        for (const auto &v : data.vertlist)
            verts.append({ v->basePoint.x, v->basePoint.y, v->bulge, v->stawidth, v->endwidth });
        if (verts.size() < 2)
            return;
        const QString layer = QString::fromStdString(data.layer);
        for (GraphicsPrimitive *p : buildFromVertices(verts, closed, ctx))
            emitPrimitive(p, layer);
    }

    void addSpline(const DRW_Spline *data) override
    {
        const auto &pts = !data->fitlist.empty() ? data->fitlist : data->controllist;
        if (pts.size() < 2)
            return;
        const bool closed = (data->flags & 1) != 0;
        const int vertexCount = qMin(int(pts.size()), int(PrimitiveComplexCurve::MaxVertices));
        if (vertexCount < int(pts.size()))
            ++ctx.truncatedVertexCount;

        auto *p = new PrimitiveComplexCurve();
        p->setClosed(closed);
        for (int i = 0; i < vertexCount; ++i)
            p->appendVertex(QPointF(pts[i]->x, pts[i]->y));
        emitPrimitive(p, QString::fromStdString(data->layer));
    }
    void addKnot(const DRW_Entity &) override {} // legacy/DWG-only, never populated for ASCII DXF

    void addText(const DRW_Text &data) override { buildText(data, false); }
    void addMText(const DRW_MText &data) override { buildText(data, true); }

    // Resolves a block reference by placing a fresh clone of every one of
    // its (already block-local-resolved, see BlockDef) entities through
    // this INSERT's own transform. Not recursive - see BlockDef's comment -
    // insertDepth is only a defensive cap in case that invariant is ever
    // violated by a future change, never expected to trigger in practice.
    void addInsert(const DRW_Insert &data) override
    {
        if (insertDepth >= kMaxInsertDepth)
            return;
        const QString blockName = QString::fromStdString(data.name);
        if (!blocks.contains(blockName)) {
            ++ctx.missingBlockCount;
            return;
        }
        const BlockDef &block = blocks.value(blockName);
        const QPointF insertPoint(data.basePoint.x, data.basePoint.y);
        const qreal xScale = qFuzzyIsNull(data.xscale) ? 1.0 : data.xscale;
        const qreal yScale = qFuzzyIsNull(data.yscale) ? 1.0 : data.yscale;
        // DRW_Insert::angle is in radians, unlike DRW_Text::angle (degrees).
        const qreal rotationDeg = qRadiansToDegrees(data.angle);

        QTransform transform;
        transform.translate(insertPoint.x(), insertPoint.y());
        transform.rotate(rotationDeg);
        transform.scale(xScale, yScale);
        transform.translate(-block.basePoint.x(), -block.basePoint.y());

        const qreal uniformScale = (qAbs(xScale) + qAbs(yScale)) / 2.0;
        ++insertDepth;
        for (const ParsedPrimitive &pp : block.entities) {
            GraphicsPrimitive *clone = clonePrimitive(pp.primitive);
            if (!clone)
                continue;
            applyInsertTransform(clone, transform, rotationDeg, uniformScale);
            emitPrimitive(clone, pp.dxfLayer);
        }
        --insertDepth;
    }

    // --- unsupported entity types: counted for the post-import summary ------
    void addRay(const DRW_Ray &) override { ++ctx.unsupportedCounts[QStringLiteral("RAY")]; }
    void addXline(const DRW_Xline &) override { ++ctx.unsupportedCounts[QStringLiteral("XLINE")]; }
    void addTrace(const DRW_Trace &) override { ++ctx.unsupportedCounts[QStringLiteral("TRACE")]; }
    void add3dFace(const DRW_3Dface &) override { ++ctx.unsupportedCounts[QStringLiteral("3DFACE")]; }
    void addSolid(const DRW_Solid &) override { ++ctx.unsupportedCounts[QStringLiteral("SOLID")]; }
    void addDimAlign(const DRW_DimAligned *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addDimLinear(const DRW_DimLinear *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addDimRadial(const DRW_DimRadial *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addDimDiametric(const DRW_DimDiametric *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addDimAngular(const DRW_DimAngular *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addDimAngular3P(const DRW_DimAngular3p *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addDimOrdinate(const DRW_DimOrdinate *) override { ++ctx.unsupportedCounts[QStringLiteral("DIMENSION")]; }
    void addLeader(const DRW_Leader *) override { ++ctx.unsupportedCounts[QStringLiteral("LEADER")]; }
    void addHatch(const DRW_Hatch *) override { ++ctx.unsupportedCounts[QStringLiteral("HATCH")]; }
    void addViewport(const DRW_Viewport &) override { ++ctx.unsupportedCounts[QStringLiteral("VIEWPORT")]; }
    void addImage(const DRW_Image *) override { ++ctx.unsupportedCounts[QStringLiteral("IMAGE")]; }

    // --- structural / irrelevant ---------------------------------------------
    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void linkImage(const DRW_ImageDef *) override {}
    void addComment(const char *) override {}
    void addPlotSettings(const DRW_PlotSettings *) override {}

    // --- write-side: never called on the read path, still pure virtual ------
    void writeHeader(DRW_Header &) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}
};

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

bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage, QStringList *warnings)
{
    sheet->clearPrimitives();

    ImportInterface iface;
    dxfRW reader(filePath.toLocal8Bit().constData());
    const bool ok = reader.read(&iface, /*ext=*/false);
    if (!ok) {
        if (errorMessage)
            *errorMessage = QStringLiteral("libdxfrw read() failed");
        return false;
    }

    QStringList usedLayerNames;
    QSet<QString> seenLayerNames;
    for (const ParsedPrimitive &pp : iface.topLevel) {
        if (!seenLayerNames.contains(pp.dxfLayer)) {
            seenLayerNames.insert(pp.dxfLayer);
            usedLayerNames.append(pp.dxfLayer);
        }
    }
    const QHash<QString, Layer *> layerMapping = remapLayers(usedLayerNames, iface.layerColors, warnings);

    for (const ParsedPrimitive &pp : iface.topLevel) {
        if (pp.primitive->isDegenerate()) {
            delete pp.primitive;
            continue;
        }
        pp.primitive->setLayer(layerMapping.value(pp.dxfLayer, LayerList::getInstance().getMaster()));
        sheet->addPrimitive(pp.primitive);
    }

    if (warnings) {
        for (auto it = iface.ctx.unsupportedCounts.constBegin(); it != iface.ctx.unsupportedCounts.constEnd(); ++it) {
            warnings->append(QObject::tr("%1 entità %2 non supportate sono state ignorate.")
                                      .arg(it.value()).arg(it.key()));
        }
        if (iface.ctx.truncatedVertexCount > 0) {
            warnings->append(QObject::tr("%1 elementi con più di %2 vertici sono stati troncati.")
                                      .arg(iface.ctx.truncatedVertexCount).arg(PrimitivePolygon::MaxVertices));
        }
        if (iface.ctx.rotatedEllipseCount > 0) {
            warnings->append(QObject::tr("%1 ellissi ruotate sono state importate come non ruotate "
                                          "(bounding box).").arg(iface.ctx.rotatedEllipseCount));
        }
        if (iface.ctx.missingBlockCount > 0) {
            warnings->append(QObject::tr("%1 riferimenti INSERT a blocchi non trovati sono stati ignorati.")
                                      .arg(iface.ctx.missingBlockCount));
        }
    }

    return true;
}

void read(const QString &text, Sheet *sheet, QStringList *warnings)
{
    // libdxfrw only reads from a real file path - route through a temp file
    // (mirrors DxfWriter::write()'s own temp-file bridge, the other way).
    QTemporaryFile temp;
    if (!temp.open()) {
        sheet->clearPrimitives();
        return;
    }
    {
        QTextStream stream(&temp);
        stream << text;
    }
    const QString path = temp.fileName();
    temp.close();

    readFile(path, sheet, nullptr, warnings);
}

} // namespace DxfReader
