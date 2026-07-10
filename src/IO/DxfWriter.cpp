/*
 * DxfWriter.cpp
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

#include "DxfWriter.h"
#include "DxfCommon.h"
#include "Sheet.h"
#include "Layer.h"
#include "PrimitiveLine.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "PrimitiveBezier.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitivePolygon.h"
#include "PrimitiveConnection.h"
#include "PrimitivePcbTrack.h"
#include "PrimitivePad.h"
#include "PrimitiveMacro.h"
#include "PrimitiveText.h"

#include "libdxfrw.h"
#include "drw_interface.h"

#include <QFile>
#include <QTemporaryFile>
#include <QMap>
#include <QtMath>

namespace DxfWriter {

namespace {

using namespace DxfCommon;

// DXF layer names can't contain <>/\":;?*|,=` or be empty.
QString sanitizeLayerName(const QString &rawName)
{
    QString name = rawName;
    static const QString forbidden = QStringLiteral("<>/\\\":;?*|,=`");
    for (const QChar &ch : forbidden)
        name.replace(ch, QLatin1Char('_'));
    return name.isEmpty() ? QStringLiteral("0") : name;
}

QString layerNameOf(GraphicsPrimitive *primitive)
{
    Layer *layer = primitive->layer();
    return sanitizeLayerName(layer ? layer->name() : QStringLiteral("0"));
}

QColor layerColorOf(GraphicsPrimitive *primitive)
{
    Layer *layer = primitive->layer();
    return layer ? layer->color() : QColor(Qt::black);
}

// Expands every PrimitiveMacro instance into loose primitives via its own
// convertToPrimitives(), re-expanding the result in case the macro body
// itself contains nested macros (up to a depth guard against malformed
// self-referential libraries). DXF export never emits BLOCKS/INSERT.
// Returns the final flat primitive list; `owned` collects every
// freshly-allocated primitive (the flattening output) so the caller can
// free them once done - the original, non-macro primitives are returned by
// pointer into the Sheet's own list and must NOT be deleted.
QList<GraphicsPrimitive *> flattenMacros(const Sheet *sheet, QList<GraphicsPrimitive *> &owned)
{
    QList<GraphicsPrimitive *> result;
    QList<GraphicsPrimitive *> pending = sheet->primitives();
    const int maxDepth = 20;

    for (int depth = 0; !pending.isEmpty() && depth < maxDepth; ++depth) {
        QList<GraphicsPrimitive *> nextPending;
        for (GraphicsPrimitive *primitive : pending) {
            if (primitive->getPrimitiveType() == GraphicsPrimitive::PartLib) {
                auto *macro = static_cast<PrimitiveMacro *>(primitive);
                // convertToPrimitives() only reads contextSheet (e.g. for a
                // nested "SA" line's default connection diameter) - never
                // mutates it, matching FidoCadReader::parse()'s own
                // read-only, non-const Sheet* contextSheet convention.
                const QList<GraphicsPrimitive *> expanded =
                        macro->convertToPrimitives(const_cast<Sheet *>(sheet));
                owned.append(expanded);
                nextPending.append(expanded);
            } else {
                result.append(primitive);
            }
        }
        pending = nextPending;
    }
    // Any macros still pending past maxDepth (a pathological self-referential
    // library) are simply dropped - better than an infinite/huge expansion.

    return result;
}

QPointF bezierPoint(qreal t, const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3)
{
    const qreal u = 1.0 - t;
    return u * u * u * p0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t * p3;
}

DRW_Coord toCoord(const QPointF &p)
{
    return DRW_Coord(p.x(), p.y(), 0.0);
}

void addPolylineVertices(DRW_LWPolyline &lwp, const QVector<QPointF> &points,
                          qreal startWidth = 0.0, qreal endWidth = 0.0)
{
    for (const QPointF &pt : points) {
        DRW_Vertex2D v(pt.x(), pt.y(), 0.0);
        v.stawidth = startWidth;
        v.endwidth = endWidth;
        lwp.addVertex(v);
    }
}

// Everything this app can write, keyed by the DRW_Interface implementation's
// writeEntities() below - one function per DXF entity kind, taking the
// dxfRW to write through plus the entity data. Kept as free functions
// (rather than DRW_Entity subclass methods) since libdxfrw's own entity
// structs are plain data, not something we subclass.
class ExportInterface : public DRW_Interface {
public:
    dxfRW *writer = nullptr;
    QList<GraphicsPrimitive *> primitives; // pre-flattened, ready to emit
    QMap<QString, QColor> layers;          // name -> color, gathered up front

    void writeEntity(GraphicsPrimitive *primitive)
    {
        if (primitive->isDegenerate())
            return;

        const std::string layerName = layerNameOf(primitive).toStdString();

        switch (primitive->getPrimitiveType()) {
        case GraphicsPrimitive::Line: {
            auto *p = static_cast<PrimitiveLine *>(primitive);
            DRW_Line ent;
            ent.layer = layerName;
            ent.basePoint = toCoord(p->controlPoint(0));
            ent.secPoint = toCoord(p->controlPoint(1));
            writer->writeLine(&ent);
            break;
        }
        case GraphicsPrimitive::Rectangle: {
            auto *p = static_cast<PrimitiveRectangle *>(primitive);
            const QPointF a = p->controlPoint(0);
            const QPointF c = p->controlPoint(1);
            const QVector<QPointF> corners { a, QPointF(c.x(), a.y()), c, QPointF(a.x(), c.y()) };
            DRW_LWPolyline ent;
            ent.layer = layerName;
            ent.flags = 1; // closed
            addPolylineVertices(ent, corners);
            writer->writeLWPolyline(&ent);
            break;
        }
        case GraphicsPrimitive::Ellipse: {
            auto *p = static_cast<PrimitiveEllipse *>(primitive);
            const QPointF a = p->controlPoint(0);
            const QPointF b = p->controlPoint(1);
            const QPointF center = (a + b) / 2.0;
            const qreal rx = qAbs(b.x() - a.x()) / 2.0;
            const qreal ry = qAbs(b.y() - a.y()) / 2.0;
            if (qFuzzyCompare(rx, ry)) {
                DRW_Circle ent;
                ent.layer = layerName;
                ent.basePoint = toCoord(center);
                ent.radious = rx;
                writer->writeCircle(&ent);
            } else {
                DRW_Ellipse ent;
                ent.layer = layerName;
                ent.basePoint = toCoord(center);
                // Major axis endpoint, relative to center, along whichever
                // of rx/ry is larger, so the ratio never exceeds 1.
                if (rx >= ry) {
                    ent.secPoint = DRW_Coord(rx, 0.0, 0.0);
                    ent.ratio = ry / rx;
                } else {
                    ent.secPoint = DRW_Coord(0.0, ry, 0.0);
                    ent.ratio = rx / ry;
                }
                ent.staparam = 0.0;
                ent.endparam = 2.0 * M_PI;
                writer->writeEllipse(&ent);
            }
            break;
        }
        case GraphicsPrimitive::Bezier: {
            // libdxfrw's writeSpline() (this vendored version) only ever
            // emits control points (group 10/20/30), never fit points
            // (11/21/31) - a fit-point SPLINE round-trips through this
            // app's own lenient reader and ezdxf but writes out with no
            // coordinate data at all, silently dropping the curve. Sampled
            // densely into a plain LWPOLYLINE instead - geometrically
            // identical to a degree-1 (linear) spline through the same
            // points, without depending on that unimplemented path.
            auto *p = static_cast<PrimitiveBezier *>(primitive);
            const QPointF p0 = p->controlPoint(0);
            const QPointF c1 = p->controlPoint(1);
            const QPointF c2 = p->controlPoint(2);
            const QPointF p3 = p->controlPoint(3);
            QVector<QPointF> points;
            const int steps = 24;
            for (int i = 0; i <= steps; ++i)
                points.append(bezierPoint(qreal(i) / steps, p0, c1, c2, p3));
            DRW_LWPolyline ent;
            ent.layer = layerName;
            ent.flags = 0; // open
            addPolylineVertices(ent, points);
            writer->writeLWPolyline(&ent);
            break;
        }
        case GraphicsPrimitive::Spline: {
            // Same LWPOLYLINE fallback as Bezier above, and for the same
            // reason. Straight segments between the curve's own vertices,
            // not a resampled smooth interpolation - a known simplification
            // (the smoothing algorithm is PrimitiveComplexCurve's own
            // private implementation detail, not exposed for reuse here).
            auto *p = static_cast<PrimitiveComplexCurve *>(primitive);
            QVector<QPointF> points;
            for (int i = 0; i < p->controlPointCount(); ++i)
                points.append(p->controlPoint(i));
            DRW_LWPolyline ent;
            ent.layer = layerName;
            ent.flags = p->isClosed() ? 1 : 0;
            addPolylineVertices(ent, points);
            writer->writeLWPolyline(&ent);
            break;
        }
        case GraphicsPrimitive::Polyline: {
            auto *p = static_cast<PrimitivePolygon *>(primitive);
            QVector<QPointF> points;
            for (int i = 0; i < p->controlPointCount(); ++i)
                points.append(p->controlPoint(i));
            DRW_LWPolyline ent;
            ent.layer = layerName;
            ent.flags = 1; // closed
            addPolylineVertices(ent, points);
            writer->writeLWPolyline(&ent);
            break;
        }
        case GraphicsPrimitive::Connection: {
            auto *p = static_cast<PrimitiveConnection *>(primitive);
            DRW_Point ent;
            ent.layer = layerName;
            ent.basePoint = toCoord(p->controlPoint(0));
            writer->writePoint(&ent);
            break;
        }
        case GraphicsPrimitive::PcbTrack: {
            // A 2-vertex widthed (open) LWPOLYLINE - the structurally
            // distinctive signature DxfReader recognizes as a
            // PrimitivePcbTrack on the way back in (a plain LINE never
            // carries width).
            auto *p = static_cast<PrimitivePcbTrack *>(primitive);
            DRW_LWPolyline ent;
            ent.layer = layerName;
            ent.flags = 0; // open
            addPolylineVertices(ent, { p->controlPoint(0), p->controlPoint(1) }, p->width(), p->width());
            writer->writeLWPolyline(&ent);
            break;
        }
        case GraphicsPrimitive::Pad: {
            auto *p = static_cast<PrimitivePad *>(primitive);
            const QPointF center = p->controlPoint(0);
            if (p->style() == PrimitivePad::Round) {
                DRW_Circle ent;
                ent.layer = layerName;
                ent.basePoint = toCoord(center);
                ent.radious = qMax(p->outerWidth(), p->outerHeight()) / 2.0;
                writer->writeCircle(&ent);
            } else {
                const qreal hw = p->outerWidth() / 2.0;
                const qreal hh = p->outerHeight() / 2.0;
                const QVector<QPointF> corners {
                    center + QPointF(-hw, -hh), center + QPointF(hw, -hh),
                    center + QPointF(hw, hh), center + QPointF(-hw, hh)
                };
                DRW_LWPolyline ent;
                ent.layer = layerName;
                ent.flags = 1; // closed
                addPolylineVertices(ent, corners);
                writer->writeLWPolyline(&ent);
            }
            break;
        }
        case GraphicsPrimitive::Text: {
            auto *p = static_cast<PrimitiveText *>(primitive);
            DRW_Text ent;
            ent.layer = layerName;
            ent.basePoint = toCoord(p->controlPoint(0));
            ent.height = p->sizeY();
            ent.text = p->text().toStdString();
            ent.angle = p->orientationDeg(); // DXF TEXT rotation (code 50) is in degrees
            writer->writeText(&ent);
            break;
        }
        case GraphicsPrimitive::PartLib:
            // Flattened away before writeEntity() is ever called on one -
            // see flattenMacros().
            break;
        case GraphicsPrimitive::Image:
            // No portable DXF equivalent (IMAGE needs an external file
            // reference, not inline data) - known, accepted limitation.
            break;
        }
    }

    // --- write-side callbacks actually used ---------------------------------
    void writeHeader(DRW_Header &) override {}
    void writeLayers() override
    {
        for (auto it = layers.constBegin(); it != layers.constEnd(); ++it) {
            DRW_Layer layer;
            layer.name = it.key().toStdString();
            layer.color = colorToAci(it.value());
            layer.lineType = "CONTINUOUS";
            writer->writeLayer(&layer);
        }
    }
    void writeEntities() override
    {
        for (GraphicsPrimitive *primitive : primitives)
            writeEntity(primitive);
    }
    // Nothing else this app writes - every one of these is a legitimate
    // no-op (dxfRW::write() supplies mandatory default table entries, e.g.
    // "0"/"Standard", on its own when these add nothing).
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeLTypes() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    // --- read-side callbacks: unused on the write path, still pure virtual --
    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addLayer(const DRW_Layer &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void addBlock(const DRW_Block &) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point &) override {}
    void addLine(const DRW_Line &) override {}
    void addRay(const DRW_Ray &) override {}
    void addXline(const DRW_Xline &) override {}
    void addArc(const DRW_Arc &) override {}
    void addCircle(const DRW_Circle &) override {}
    void addEllipse(const DRW_Ellipse &) override {}
    void addLWPolyline(const DRW_LWPolyline &) override {}
    void addPolyline(const DRW_Polyline &) override {}
    void addSpline(const DRW_Spline *) override {}
    void addKnot(const DRW_Entity &) override {}
    void addInsert(const DRW_Insert &) override {}
    void addTrace(const DRW_Trace &) override {}
    void add3dFace(const DRW_3Dface &) override {}
    void addSolid(const DRW_Solid &) override {}
    void addMText(const DRW_MText &) override {}
    void addText(const DRW_Text &) override {}
    void addDimAlign(const DRW_DimAligned *) override {}
    void addDimLinear(const DRW_DimLinear *) override {}
    void addDimRadial(const DRW_DimRadial *) override {}
    void addDimDiametric(const DRW_DimDiametric *) override {}
    void addDimAngular(const DRW_DimAngular *) override {}
    void addDimAngular3P(const DRW_DimAngular3p *) override {}
    void addDimOrdinate(const DRW_DimOrdinate *) override {}
    void addLeader(const DRW_Leader *) override {}
    void addHatch(const DRW_Hatch *) override {}
    void addViewport(const DRW_Viewport &) override {}
    void addImage(const DRW_Image *) override {}
    void linkImage(const DRW_ImageDef *) override {}
    void addComment(const char *) override {}
    void addPlotSettings(const DRW_PlotSettings *) override {}
};

} // namespace

bool writeFile(const Sheet *sheet, const QString &filePath, QString *errorMessage)
{
    QList<GraphicsPrimitive *> owned;
    ExportInterface iface;
    iface.primitives = flattenMacros(sheet, owned);

    iface.layers.insert(QStringLiteral("0"), QColor(Qt::white));
    for (GraphicsPrimitive *primitive : iface.primitives) {
        if (primitive->isDegenerate())
            continue;
        iface.layers.insert(layerNameOf(primitive), layerColorOf(primitive));
    }

    dxfRW writer(filePath.toLocal8Bit().constData());
    iface.writer = &writer;
    const bool ok = writer.write(&iface, DRW::AC1015, /*bin=*/false);

    qDeleteAll(owned);

    if (!ok && errorMessage)
        *errorMessage = QStringLiteral("libdxfrw write() failed");
    return ok;
}

QString write(const Sheet *sheet)
{
    // libdxfrw always writes straight to a file path - there is no
    // in-memory writer to call directly, unlike FidoCadWriter's own
    // hand-rolled QStringList-based one. Route through a temp file instead,
    // so callers that want the text (e.g. tests) can still get it.
    QTemporaryFile temp;
    if (!temp.open())
        return QString();
    const QString path = temp.fileName();
    temp.close();

    QString error;
    if (!writeFile(sheet, path, &error))
        return QString();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

} // namespace DxfWriter
