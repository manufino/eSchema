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

#include <QFile>
#include <QTextStream>
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

void writeHeaderSection(QStringList &lines)
{
    lines << QStringLiteral("0") << QStringLiteral("SECTION");
    appendGroup(lines, 2, QStringLiteral("HEADER"));
    appendGroup(lines, 9, QStringLiteral("$ACADVER"));
    appendGroup(lines, 1, QStringLiteral("AC1015"));
    appendGroup(lines, 9, QStringLiteral("$INSUNITS"));
    appendGroup(lines, 70, 0); // unitless, matching eSchema's own unit-agnostic coordinates
    lines << QStringLiteral("0") << QStringLiteral("ENDSEC");
}

void writeTablesSection(QStringList &lines, const QMap<QString, QColor> &layers)
{
    lines << QStringLiteral("0") << QStringLiteral("SECTION");
    appendGroup(lines, 2, QStringLiteral("TABLES"));

    lines << QStringLiteral("0") << QStringLiteral("TABLE");
    appendGroup(lines, 2, QStringLiteral("LAYER"));
    appendGroup(lines, 70, int(layers.size()));

    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it) {
        lines << QStringLiteral("0") << QStringLiteral("LAYER");
        appendGroup(lines, 2, it.key());
        appendGroup(lines, 70, 0);
        appendGroup(lines, 62, colorToAci(it.value()));
        appendGroup(lines, 6, QStringLiteral("CONTINUOUS"));
    }

    lines << QStringLiteral("0") << QStringLiteral("ENDTAB");
    lines << QStringLiteral("0") << QStringLiteral("ENDSEC");
}

void writeLine(QStringList &lines, const QPointF &p1, const QPointF &p2, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("LINE");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 10, p1.x());
    appendGroup(lines, 20, p1.y());
    appendGroup(lines, 30, 0.0);
    appendGroup(lines, 11, p2.x());
    appendGroup(lines, 21, p2.y());
    appendGroup(lines, 31, 0.0);
}

void writeCircle(QStringList &lines, const QPointF &center, qreal radius, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("CIRCLE");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 10, center.x());
    appendGroup(lines, 20, center.y());
    appendGroup(lines, 30, 0.0);
    appendGroup(lines, 40, radius);
}

void writeEllipse(QStringList &lines, const QPointF &center, qreal rx, qreal ry, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("ELLIPSE");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 10, center.x());
    appendGroup(lines, 20, center.y());
    appendGroup(lines, 30, 0.0);
    // Major axis endpoint, relative to center - along whichever of rx/ry is
    // larger, so the ratio (group 40) never exceeds 1.
    if (rx >= ry) {
        appendGroup(lines, 11, rx);
        appendGroup(lines, 21, 0.0);
        appendGroup(lines, 40, ry / rx);
    } else {
        appendGroup(lines, 11, 0.0);
        appendGroup(lines, 21, ry);
        appendGroup(lines, 40, rx / ry);
    }
    appendGroup(lines, 31, 0.0);
    appendGroup(lines, 41, 0.0);              // start parameter
    appendGroup(lines, 42, 2.0 * M_PI);       // end parameter - full ellipse
}

// Closed polygon of arbitrary vertex count (PrimitiveRectangle/PrimitivePolygon
// and PrimitivePad's rectangular styles all funnel through this).
void writeClosedPolyline(QStringList &lines, const QVector<QPointF> &vertices, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("LWPOLYLINE");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 100, QStringLiteral("AcDbEntity"));
    appendGroup(lines, 100, QStringLiteral("AcDbPolyline"));
    appendGroup(lines, 90, int(vertices.size()));
    appendGroup(lines, 70, 1); // bit 0 = closed
    for (const QPointF &v : vertices) {
        appendGroup(lines, 10, v.x());
        appendGroup(lines, 20, v.y());
    }
}

// A 2-vertex widthed (open) LWPOLYLINE - the structurally distinctive
// signature DxfReader recognizes as a PrimitivePcbTrack on the way back in.
void writeTrackPolyline(QStringList &lines, const QPointF &p1, const QPointF &p2,
                         qreal width, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("LWPOLYLINE");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 100, QStringLiteral("AcDbEntity"));
    appendGroup(lines, 100, QStringLiteral("AcDbPolyline"));
    appendGroup(lines, 90, 2);
    appendGroup(lines, 70, 0); // open
    appendGroup(lines, 10, p1.x());
    appendGroup(lines, 20, p1.y());
    appendGroup(lines, 40, width);
    appendGroup(lines, 41, width);
    appendGroup(lines, 10, p2.x());
    appendGroup(lines, 20, p2.y());
    appendGroup(lines, 40, width);
    appendGroup(lines, 41, width);
}

// Fit-point spline - used both for PrimitiveComplexCurve (its own vertices
// are the fit points directly) and for PrimitiveBezier (sampled into points
// first, see bezierPoint() below), so both share this one, hand-verified
// group-code combination rather than also emitting a second, riskier
// control-point spline form.
void writeFitPointSpline(QStringList &lines, const QVector<QPointF> &fitPoints, bool closed, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("SPLINE");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 100, QStringLiteral("AcDbEntity"));
    appendGroup(lines, 100, QStringLiteral("AcDbSpline"));
    appendGroup(lines, 70, closed ? 9 : 8); // 8 = planar, +1 = closed
    appendGroup(lines, 71, 3);              // degree
    appendGroup(lines, 72, 0);              // knot count - none, fit-point form
    appendGroup(lines, 73, 0);              // control point count - none
    appendGroup(lines, 74, int(fitPoints.size()));
    for (const QPointF &v : fitPoints) {
        appendGroup(lines, 11, v.x());
        appendGroup(lines, 21, v.y());
        appendGroup(lines, 31, 0.0);
    }
}

QPointF bezierPoint(qreal t, const QPointF &p0, const QPointF &p1, const QPointF &p2, const QPointF &p3)
{
    const qreal u = 1.0 - t;
    return u * u * u * p0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t * p3;
}

void writePoint(QStringList &lines, const QPointF &pos, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("POINT");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 10, pos.x());
    appendGroup(lines, 20, pos.y());
    appendGroup(lines, 30, 0.0);
}

void writeText(QStringList &lines, const QPointF &pos, qreal height, qreal rotationDeg,
               const QString &text, const QString &layerName)
{
    lines << QStringLiteral("0") << QStringLiteral("TEXT");
    appendGroup(lines, 8, layerName);
    appendGroup(lines, 10, pos.x());
    appendGroup(lines, 20, pos.y());
    appendGroup(lines, 30, 0.0);
    appendGroup(lines, 40, height);
    appendGroup(lines, 1, text);
    appendGroup(lines, 50, rotationDeg);
}

void writePrimitive(QStringList &lines, GraphicsPrimitive *primitive)
{
    if (primitive->isDegenerate())
        return;

    const QString layerName = layerNameOf(primitive);

    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Line: {
        auto *p = static_cast<PrimitiveLine *>(primitive);
        writeLine(lines, p->controlPoint(0), p->controlPoint(1), layerName);
        break;
    }
    case GraphicsPrimitive::Rectangle: {
        auto *p = static_cast<PrimitiveRectangle *>(primitive);
        const QPointF a = p->controlPoint(0);
        const QPointF c = p->controlPoint(1);
        const QVector<QPointF> corners { a, QPointF(c.x(), a.y()), c, QPointF(a.x(), c.y()) };
        writeClosedPolyline(lines, corners, layerName);
        break;
    }
    case GraphicsPrimitive::Ellipse: {
        auto *p = static_cast<PrimitiveEllipse *>(primitive);
        const QPointF a = p->controlPoint(0);
        const QPointF b = p->controlPoint(1);
        const QPointF center = (a + b) / 2.0;
        const qreal rx = qAbs(b.x() - a.x()) / 2.0;
        const qreal ry = qAbs(b.y() - a.y()) / 2.0;
        if (qFuzzyCompare(rx, ry))
            writeCircle(lines, center, rx, layerName);
        else
            writeEllipse(lines, center, rx, ry, layerName);
        break;
    }
    case GraphicsPrimitive::Bezier: {
        auto *p = static_cast<PrimitiveBezier *>(primitive);
        const QPointF p0 = p->controlPoint(0);
        const QPointF c1 = p->controlPoint(1);
        const QPointF c2 = p->controlPoint(2);
        const QPointF p3 = p->controlPoint(3);
        // Sampled into a fit-point spline rather than emitted as an exact
        // 4-control-point spline, to reuse the one hand-verified SPLINE
        // group-code combination (see writeFitPointSpline()) instead of a
        // second, unvalidated one.
        QVector<QPointF> points;
        const int steps = 24;
        for (int i = 0; i <= steps; ++i)
            points.append(bezierPoint(qreal(i) / steps, p0, c1, c2, p3));
        writeFitPointSpline(lines, points, false, layerName);
        break;
    }
    case GraphicsPrimitive::Spline: {
        auto *p = static_cast<PrimitiveComplexCurve *>(primitive);
        QVector<QPointF> points;
        for (int i = 0; i < p->controlPointCount(); ++i)
            points.append(p->controlPoint(i));
        writeFitPointSpline(lines, points, p->isClosed(), layerName);
        break;
    }
    case GraphicsPrimitive::Polyline: {
        auto *p = static_cast<PrimitivePolygon *>(primitive);
        QVector<QPointF> points;
        for (int i = 0; i < p->controlPointCount(); ++i)
            points.append(p->controlPoint(i));
        writeClosedPolyline(lines, points, layerName);
        break;
    }
    case GraphicsPrimitive::Connection: {
        auto *p = static_cast<PrimitiveConnection *>(primitive);
        writePoint(lines, p->controlPoint(0), layerName);
        break;
    }
    case GraphicsPrimitive::PcbTrack: {
        auto *p = static_cast<PrimitivePcbTrack *>(primitive);
        writeTrackPolyline(lines, p->controlPoint(0), p->controlPoint(1), p->width(), layerName);
        break;
    }
    case GraphicsPrimitive::Pad: {
        auto *p = static_cast<PrimitivePad *>(primitive);
        const QPointF center = p->controlPoint(0);
        if (p->style() == PrimitivePad::Round) {
            writeCircle(lines, center, qMax(p->outerWidth(), p->outerHeight()) / 2.0, layerName);
        } else {
            const qreal hw = p->outerWidth() / 2.0;
            const qreal hh = p->outerHeight() / 2.0;
            const QVector<QPointF> corners {
                center + QPointF(-hw, -hh), center + QPointF(hw, -hh),
                center + QPointF(hw, hh), center + QPointF(-hw, hh)
            };
            writeClosedPolyline(lines, corners, layerName);
        }
        break;
    }
    case GraphicsPrimitive::Text: {
        auto *p = static_cast<PrimitiveText *>(primitive);
        writeText(lines, p->controlPoint(0), p->sizeY(), p->orientationDeg(), p->text(), layerName);
        break;
    }
    case GraphicsPrimitive::PartLib:
        // Flattened away before writePrimitive() is ever called on one - see
        // flattenMacros().
        break;
    case GraphicsPrimitive::Image:
        // No portable DXF equivalent (IMAGE needs an external file
        // reference, not inline data) - known, accepted limitation.
        break;
    }
}

} // namespace

QString write(const Sheet *sheet)
{
    QList<GraphicsPrimitive *> owned;
    const QList<GraphicsPrimitive *> primitives = flattenMacros(sheet, owned);

    QMap<QString, QColor> layers;
    layers.insert(QStringLiteral("0"), QColor(Qt::white));
    for (GraphicsPrimitive *primitive : primitives) {
        if (primitive->isDegenerate())
            continue;
        layers.insert(layerNameOf(primitive), layerColorOf(primitive));
    }

    QStringList lines;
    writeHeaderSection(lines);
    writeTablesSection(lines, layers);

    lines << QStringLiteral("0") << QStringLiteral("SECTION");
    appendGroup(lines, 2, QStringLiteral("ENTITIES"));
    for (GraphicsPrimitive *primitive : primitives)
        writePrimitive(lines, primitive);
    lines << QStringLiteral("0") << QStringLiteral("ENDSEC");

    lines << QStringLiteral("0") << QStringLiteral("EOF");

    qDeleteAll(owned);

    return lines.join(QLatin1Char('\n'));
}

bool writeFile(const Sheet *sheet, const QString &filePath, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream << write(sheet);
    return true;
}

} // namespace DxfWriter
