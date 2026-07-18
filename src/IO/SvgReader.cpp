/*
 * SvgReader.cpp
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

#include "SvgReader.h"
#include "Sheet.h"
#include "BooleanOps.h"
#include "GraphicsPrimitive.h"
#include "PrimitiveLine.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "PrimitivePolygon.h"
#include "PrimitiveBezier.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitiveText.h"
#include "LayerList.h"
#include "Layer.h"

#include <QXmlStreamReader>
#include <QFile>
#include <QHash>
#include <QPainterPath>
#include <QPolygonF>
#include <QRegularExpression>
#include <QTextStream>
#include <QTransform>
#include <QtMath>
#include <cmath>

namespace {

// CSS pixels are defined as 1/96 inch; the FidoCadJ logical unit is 1/200
// inch - this uniform root scale keeps imported drawings physically sized.
constexpr qreal UnitsPerPixel = 200.0 / 96.0;

// The style/transform state inherited down the g/svg nesting. SVG's own
// defaults apply: fill=black, stroke=none - a bare <rect> is a filled
// black rectangle.
struct SvgState {
    QTransform transform;
    QColor stroke;              // invalid = none
    QColor fill = QColor(Qt::black);
    bool display = true;        // display:none hides the whole subtree
};

// Assigns global layer slots to the distinct colors actually used, exactly
// like DxfReader does for DXF layers: black keeps layer 0, each new color
// claims the next slot 1-15 (renamed and recolored); past 15 everything
// falls back to layer 0 with one summary warning.
class LayerAssigner
{
public:
    Layer *layerFor(const QColor &color)
    {
        QList<Layer *> *layers = LayerList::getInstance().getList();
        if (!color.isValid() || color.rgb() == QColor(Qt::black).rgb() || layers->isEmpty())
            return layers->isEmpty() ? nullptr : layers->at(0);
        const auto assigned = m_assigned.constFind(color.rgb());
        if (assigned != m_assigned.constEnd())
            return layers->at(assigned.value());
        if (m_nextSlot >= qMin(16, layers->size())) {
            ++overflowCount;
            return layers->at(0);
        }
        Layer *layer = layers->at(m_nextSlot);
        layer->setName(QStringLiteral("SVG %1").arg(color.name()));
        layer->setColor(color);
        m_assigned.insert(color.rgb(), m_nextSlot);
        ++m_nextSlot;
        return layer;
    }

    bool touchedLayers() const { return !m_assigned.isEmpty(); }
    int overflowCount = 0;

private:
    QHash<QRgb, int> m_assigned;
    int m_nextSlot = 1;
};

// "12", "12px", "10mm", "1in", "2.5cm", "8pt" -> CSS pixels.
qreal lengthToPixels(const QString &raw, bool *ok)
{
    QString text = raw.trimmed();
    qreal factor = 1.0;
    struct { const char *suffix; qreal factor; } units[] = {
        { "px", 1.0 }, { "mm", 96.0 / 25.4 }, { "cm", 96.0 / 2.54 },
        { "in", 96.0 }, { "pt", 96.0 / 72.0 }, { "pc", 16.0 },
    };
    for (const auto &unit : units) {
        if (text.endsWith(QLatin1String(unit.suffix), Qt::CaseInsensitive)) {
            text.chop(2);
            factor = unit.factor;
            break;
        }
    }
    const qreal value = text.toDouble(ok);
    return value * factor;
}

// "red", "#rrggbb", "rgb(1,2,3)" / "rgb(10%,...)"; none/transparent and
// anything unresolvable (currentColor, url(#gradient)) yield an invalid
// color - the caller decides the fallback.
QColor parseColor(const QString &raw)
{
    const QString text = raw.trimmed();
    if (text.isEmpty() || text.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0
            || text.compare(QLatin1String("transparent"), Qt::CaseInsensitive) == 0
            || text.startsWith(QLatin1String("url(")))
        return QColor();
    if (text.startsWith(QLatin1String("rgb("), Qt::CaseInsensitive) && text.endsWith(QLatin1Char(')'))) {
        const QStringList parts = text.mid(4, text.size() - 5).split(QLatin1Char(','));
        if (parts.size() != 3)
            return QColor();
        int rgb[3];
        for (int i = 0; i < 3; ++i) {
            QString part = parts.at(i).trimmed();
            if (part.endsWith(QLatin1Char('%')))
                rgb[i] = qRound(part.chopped(1).toDouble() * 2.55);
            else
                rgb[i] = part.toInt();
            rgb[i] = qBound(0, rgb[i], 255);
        }
        return QColor(rgb[0], rgb[1], rgb[2]);
    }
    return QColor(text); // #hex and named colors
}

// The numbers inside "points"/transform argument lists: comma- or
// whitespace-separated.
QVector<qreal> parseNumberList(const QString &text)
{
    QVector<qreal> numbers;
    static const QRegularExpression separator(QStringLiteral("[\\s,]+"));
    const QStringList parts = text.split(separator, Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        bool ok = false;
        const qreal value = part.toDouble(&ok);
        if (ok)
            numbers.append(value);
    }
    return numbers;
}

// The element's own transform attribute as a QTransform. SVG applies the
// listed operations right-to-left to the point; with Qt's row-vector
// convention that means composing each op in listed order in front of the
// accumulated one.
QTransform parseTransform(const QString &text)
{
    QTransform result;
    static const QRegularExpression opPattern(
            QStringLiteral("(\\w+)\\s*\\(([^)]*)\\)"));
    auto it = opPattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString op = match.captured(1);
        const QVector<qreal> a = parseNumberList(match.captured(2));
        QTransform t;
        if (op == QLatin1String("translate") && !a.isEmpty())
            t = QTransform::fromTranslate(a.at(0), a.size() > 1 ? a.at(1) : 0.0);
        else if (op == QLatin1String("scale") && !a.isEmpty())
            t = QTransform::fromScale(a.at(0), a.size() > 1 ? a.at(1) : a.at(0));
        else if (op == QLatin1String("rotate") && !a.isEmpty()) {
            if (a.size() >= 3)
                t = QTransform::fromTranslate(-a.at(1), -a.at(2))
                        * QTransform().rotate(a.at(0))
                        * QTransform::fromTranslate(a.at(1), a.at(2));
            else
                t.rotate(a.at(0));
        } else if (op == QLatin1String("matrix") && a.size() >= 6)
            t = QTransform(a.at(0), a.at(1), a.at(2), a.at(3), a.at(4), a.at(5));
        else if (op == QLatin1String("skewX") && !a.isEmpty())
            t.shear(std::tan(qDegreesToRadians(a.at(0))), 0);
        else if (op == QLatin1String("skewY") && !a.isEmpty())
            t.shear(0, std::tan(qDegreesToRadians(a.at(0))));
        else
            continue;
        result = t * result;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Path data ("d" attribute) parsing.

// One subpath (from a moveto to the next moveto/end), tracked both as a
// QPainterPath (for curved content) and enough metadata to pick the best
// FCD primitive for it afterwards.
struct Subpath {
    QPainterPath path;
    bool closed = false;
    bool curved = false;
    int segmentCount = 0;
    int cubicCount = 0;
    QPointF cubic[4]; // the lone cubic's points, when segmentCount == cubicCount == 1
};

// SVG spec F.6: endpoint-parameterized elliptical arc, appended to `path`
// as cubic segments of at most 90 degrees each.
void appendArc(QPainterPath &path, const QPointF &start, qreal rx, qreal ry,
               qreal xAxisRotationDeg, bool largeArc, bool sweep, const QPointF &end)
{
    if (qFuzzyCompare(start.x(), end.x()) && qFuzzyCompare(start.y(), end.y()))
        return;
    rx = std::abs(rx);
    ry = std::abs(ry);
    if (rx < 1e-9 || ry < 1e-9) {
        path.lineTo(end); // degenerate radii: the spec says draw a line
        return;
    }
    const qreal phi = qDegreesToRadians(xAxisRotationDeg);
    const qreal cosPhi = std::cos(phi), sinPhi = std::sin(phi);
    // Move to the ellipse-aligned frame.
    const qreal dx2 = (start.x() - end.x()) / 2.0;
    const qreal dy2 = (start.y() - end.y()) / 2.0;
    const qreal x1p = cosPhi * dx2 + sinPhi * dy2;
    const qreal y1p = -sinPhi * dx2 + cosPhi * dy2;
    // Scale radii up if the endpoints can't be joined with the given ones.
    const qreal lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1.0) {
        const qreal scale = std::sqrt(lambda);
        rx *= scale;
        ry *= scale;
    }
    // Center in the aligned frame.
    qreal sign = (largeArc != sweep) ? 1.0 : -1.0;
    qreal sq = ((rx * rx) * (ry * ry) - (rx * rx) * (y1p * y1p) - (ry * ry) * (x1p * x1p))
            / ((rx * rx) * (y1p * y1p) + (ry * ry) * (x1p * x1p));
    sq = qMax<qreal>(0.0, sq);
    const qreal coefficient = sign * std::sqrt(sq);
    const qreal cxp = coefficient * (rx * y1p / ry);
    const qreal cyp = coefficient * -(ry * x1p / rx);
    const QPointF center(cosPhi * cxp - sinPhi * cyp + (start.x() + end.x()) / 2.0,
                         sinPhi * cxp + cosPhi * cyp + (start.y() + end.y()) / 2.0);
    // Start angle and sweep extent.
    auto angleOf = [](qreal ux, qreal uy, qreal vx, qreal vy) {
        const qreal dot = ux * vx + uy * vy;
        const qreal len = std::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
        qreal angle = std::acos(qBound<qreal>(-1.0, dot / len, 1.0));
        if (ux * vy - uy * vx < 0)
            angle = -angle;
        return angle;
    };
    const qreal theta1 = angleOf(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry);
    qreal deltaTheta = angleOf((x1p - cxp) / rx, (y1p - cyp) / ry,
                               (-x1p - cxp) / rx, (-y1p - cyp) / ry);
    if (!sweep && deltaTheta > 0)
        deltaTheta -= 2 * M_PI;
    else if (sweep && deltaTheta < 0)
        deltaTheta += 2 * M_PI;
    // Split into <= 90 degree cubic segments.
    const int segments = qMax(1, qCeil(std::abs(deltaTheta) / (M_PI / 2.0)));
    const qreal delta = deltaTheta / segments;
    const qreal alpha = 4.0 / 3.0 * std::tan(delta / 4.0);
    qreal theta = theta1;
    QPointF current = start;
    for (int i = 0; i < segments; ++i) {
        const qreal nextTheta = theta + delta;
        auto pointAt = [&](qreal t) {
            const qreal x = rx * std::cos(t), y = ry * std::sin(t);
            return QPointF(cosPhi * x - sinPhi * y + center.x(),
                           sinPhi * x + cosPhi * y + center.y());
        };
        auto derivativeAt = [&](qreal t) {
            const qreal x = -rx * std::sin(t), y = ry * std::cos(t);
            return QPointF(cosPhi * x - sinPhi * y, sinPhi * x + cosPhi * y);
        };
        const QPointF next = pointAt(nextTheta);
        const QPointF c1 = current + alpha * derivativeAt(theta);
        const QPointF c2 = next - alpha * derivativeAt(nextTheta);
        path.cubicTo(c1, c2, next);
        theta = nextTheta;
        current = next;
    }
}

// Streaming tokenizer over the "d" attribute's mix of command letters,
// numbers and separators.
class PathDataStream
{
public:
    explicit PathDataStream(const QString &data) : m_data(data) {}

    bool atEnd()
    {
        skipSeparators();
        return m_index >= m_data.size();
    }
    // A command letter, or 0 when the next token is a number (implicit
    // command repetition).
    QChar peekCommand()
    {
        skipSeparators();
        if (m_index < m_data.size() && m_data.at(m_index).isLetter())
            return m_data.at(m_index);
        return QChar();
    }
    QChar takeCommand()
    {
        const QChar command = peekCommand();
        if (!command.isNull())
            ++m_index;
        return command;
    }
    bool readNumber(qreal &value)
    {
        skipSeparators();
        const int start = m_index;
        if (m_index < m_data.size() && (m_data.at(m_index) == QLatin1Char('+')
                || m_data.at(m_index) == QLatin1Char('-')))
            ++m_index;
        bool seenDot = false, seenDigit = false;
        while (m_index < m_data.size()) {
            const QChar c = m_data.at(m_index);
            if (c.isDigit()) { seenDigit = true; ++m_index; }
            else if (c == QLatin1Char('.') && !seenDot) { seenDot = true; ++m_index; }
            else if ((c == QLatin1Char('e') || c == QLatin1Char('E')) && seenDigit) {
                ++m_index;
                if (m_index < m_data.size() && (m_data.at(m_index) == QLatin1Char('+')
                        || m_data.at(m_index) == QLatin1Char('-')))
                    ++m_index;
            } else {
                break;
            }
        }
        if (!seenDigit) { m_index = start; return false; }
        bool ok = false;
        value = m_data.mid(start, m_index - start).toDouble(&ok);
        return ok;
    }
    // Arc flags may be packed without separators ("...011,5 3"): a flag is
    // always exactly one 0/1 digit.
    bool readFlag(bool &flag)
    {
        skipSeparators();
        if (m_index >= m_data.size())
            return false;
        const QChar c = m_data.at(m_index);
        if (c != QLatin1Char('0') && c != QLatin1Char('1'))
            return false;
        flag = (c == QLatin1Char('1'));
        ++m_index;
        return true;
    }

private:
    void skipSeparators()
    {
        while (m_index < m_data.size()
               && (m_data.at(m_index).isSpace() || m_data.at(m_index) == QLatin1Char(',')))
            ++m_index;
    }
    const QString m_data;
    int m_index = 0;
};

// Parses a full "d" attribute into subpaths. Returns false (bad data) only
// when nothing at all could be read.
QList<Subpath> parsePathData(const QString &data)
{
    QList<Subpath> subpaths;
    PathDataStream stream(data);
    QPointF current, subpathStart, lastControl;
    QChar lastCommand;
    Subpath *sub = nullptr;

    auto beginSubpath = [&](const QPointF &start) {
        subpaths.append(Subpath());
        sub = &subpaths.last();
        sub->path.moveTo(start);
        subpathStart = start;
        current = start;
    };
    auto lineSegment = [&](const QPointF &to) {
        if (!sub) beginSubpath(current);
        sub->path.lineTo(to);
        ++sub->segmentCount;
        current = to;
    };
    auto cubicSegment = [&](const QPointF &c1, const QPointF &c2, const QPointF &to) {
        if (!sub) beginSubpath(current);
        sub->path.cubicTo(c1, c2, to);
        sub->curved = true;
        ++sub->segmentCount;
        if (++sub->cubicCount == 1) {
            sub->cubic[0] = current; sub->cubic[1] = c1;
            sub->cubic[2] = c2; sub->cubic[3] = to;
        }
        lastControl = c2;
        current = to;
    };

    while (!stream.atEnd()) {
        QChar command = stream.takeCommand();
        if (command.isNull())
            command = lastCommand; // implicit repetition
        if (command.isNull())
            break;
        // An implicit repetition of moveto is a lineto (spec).
        if (lastCommand == command && (command == QLatin1Char('M') || command == QLatin1Char('m')))
            command = command == QLatin1Char('M') ? QLatin1Char('L') : QLatin1Char('l');

        const bool relative = command.isLower();
        const QChar op = command.toUpper();
        qreal a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;

        if (op == QLatin1Char('M')) {
            if (!stream.readNumber(a) || !stream.readNumber(b))
                break;
            const QPointF to = relative ? current + QPointF(a, b) : QPointF(a, b);
            beginSubpath(to);
        } else if (op == QLatin1Char('L')) {
            if (!stream.readNumber(a) || !stream.readNumber(b))
                break;
            lineSegment(relative ? current + QPointF(a, b) : QPointF(a, b));
        } else if (op == QLatin1Char('H')) {
            if (!stream.readNumber(a))
                break;
            lineSegment(QPointF(relative ? current.x() + a : a, current.y()));
        } else if (op == QLatin1Char('V')) {
            if (!stream.readNumber(a))
                break;
            lineSegment(QPointF(current.x(), relative ? current.y() + a : a));
        } else if (op == QLatin1Char('C')) {
            if (!stream.readNumber(a) || !stream.readNumber(b) || !stream.readNumber(c)
                    || !stream.readNumber(d) || !stream.readNumber(e) || !stream.readNumber(f))
                break;
            const QPointF base = relative ? current : QPointF(0, 0);
            cubicSegment(base + QPointF(a, b), base + QPointF(c, d), base + QPointF(e, f));
        } else if (op == QLatin1Char('S')) {
            if (!stream.readNumber(a) || !stream.readNumber(b)
                    || !stream.readNumber(c) || !stream.readNumber(d))
                break;
            const QChar prev = lastCommand.toUpper();
            const QPointF c1 = (prev == QLatin1Char('C') || prev == QLatin1Char('S'))
                    ? current * 2 - lastControl : current;
            const QPointF base = relative ? current : QPointF(0, 0);
            cubicSegment(c1, base + QPointF(a, b), base + QPointF(c, d));
        } else if (op == QLatin1Char('Q') || op == QLatin1Char('T')) {
            QPointF quadControl;
            if (op == QLatin1Char('Q')) {
                if (!stream.readNumber(a) || !stream.readNumber(b)
                        || !stream.readNumber(c) || !stream.readNumber(d))
                    break;
                const QPointF base = relative ? current : QPointF(0, 0);
                quadControl = base + QPointF(a, b);
                e = (base + QPointF(c, d)).x();
                f = (base + QPointF(c, d)).y();
            } else {
                if (!stream.readNumber(a) || !stream.readNumber(b))
                    break;
                const QChar prev = lastCommand.toUpper();
                quadControl = (prev == QLatin1Char('Q') || prev == QLatin1Char('T'))
                        ? current * 2 - lastControl : current;
                const QPointF base = relative ? current : QPointF(0, 0);
                e = (base + QPointF(a, b)).x();
                f = (base + QPointF(a, b)).y();
            }
            // Quadratic raised to cubic.
            const QPointF end(e, f);
            const QPointF c1 = current + (quadControl - current) * (2.0 / 3.0);
            const QPointF c2 = end + (quadControl - end) * (2.0 / 3.0);
            cubicSegment(c1, c2, end);
            lastControl = quadControl; // T reflects the QUADRATIC control
        } else if (op == QLatin1Char('A')) {
            bool largeArc = false, sweep = false;
            if (!stream.readNumber(a) || !stream.readNumber(b) || !stream.readNumber(c)
                    || !stream.readFlag(largeArc) || !stream.readFlag(sweep)
                    || !stream.readNumber(e) || !stream.readNumber(f))
                break;
            const QPointF to = relative ? current + QPointF(e, f) : QPointF(e, f);
            if (!sub) beginSubpath(current);
            appendArc(sub->path, current, a, b, c, largeArc, sweep, to);
            sub->curved = true;
            sub->segmentCount += 2; // never mistaken for a lone cubic
            current = to;
        } else if (op == QLatin1Char('Z')) {
            if (sub) {
                sub->path.closeSubpath();
                sub->closed = true;
            }
            current = subpathStart;
            // A new segment after Z without an explicit moveto starts a
            // fresh subpath from the same start point (spec).
            sub = nullptr;
        } else {
            break; // unknown command: stop parsing this path
        }
        lastCommand = command;
    }
    return subpaths;
}

// ---------------------------------------------------------------------------

// The whole import pass: walks the XML, maintains the state stack, and
// appends primitives to the sheet.
class Importer
{
public:
    Importer(Sheet *sheet, QStringList *warnings)
        : m_sheet(sheet), m_warnings(warnings) {}

    void run(const QString &text)
    {
        QXmlStreamReader xml(text);
        QList<SvgState> stack;
        stack.append(SvgState());
        // The uniform CSS-px -> drawing-units root scale.
        stack.last().transform = QTransform::fromScale(UnitsPerPixel, UnitsPerPixel);

        while (!xml.atEnd()) {
            const QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                const QString tag = xml.name().toString();
                SvgState state = applyAttributes(stack.last(), xml);
                if (!state.display) {
                    xml.skipCurrentElement();
                    continue;
                }
                if (tag == QLatin1String("svg")) {
                    state.transform = viewBoxTransform(xml) * state.transform;
                    stack.append(state);
                } else if (tag == QLatin1String("g") || tag == QLatin1String("a")) {
                    stack.append(state);
                } else if (tag == QLatin1String("defs") || tag == QLatin1String("symbol")
                           || tag == QLatin1String("clipPath") || tag == QLatin1String("mask")
                           || tag == QLatin1String("marker") || tag == QLatin1String("style")
                           || tag == QLatin1String("metadata") || tag == QLatin1String("title")
                           || tag == QLatin1String("desc")
                           || tag.contains(QLatin1String("radient"))
                           || tag == QLatin1String("pattern") || tag == QLatin1String("filter")) {
                    // Definitions and non-rendered content: skip silently
                    // (their referencing users are what gets counted).
                    xml.skipCurrentElement();
                } else if (handleShape(tag, xml, state)) {
                    // handleShape consumed the element (including skipping
                    // to its end where needed).
                } else {
                    countSkipped(tag);
                    xml.skipCurrentElement();
                }
            } else if (token == QXmlStreamReader::EndElement) {
                const QString tag = xml.name().toString();
                if ((tag == QLatin1String("svg") || tag == QLatin1String("g")
                     || tag == QLatin1String("a")) && stack.size() > 1)
                    stack.removeLast();
            }
        }

        flushWarnings();
    }

    bool sawAnyShape = false;

private:
    // Presentation attributes + inline style + transform, folded into the
    // inherited state.
    SvgState applyAttributes(const SvgState &parent, const QXmlStreamReader &xml)
    {
        SvgState state = parent;
        const QXmlStreamAttributes attributes = xml.attributes();
        auto applyProperty = [&state](const QString &name, const QString &value) {
            if (name == QLatin1String("stroke"))
                state.stroke = parseColor(value);
            else if (name == QLatin1String("fill"))
                state.fill = parseColor(value);
            else if (name == QLatin1String("display")
                     && value.trimmed() == QLatin1String("none"))
                state.display = false;
            else if (name == QLatin1String("visibility")
                     && value.trimmed() == QLatin1String("hidden"))
                state.display = false;
        };
        for (const QXmlStreamAttribute &attribute : attributes) {
            const QString name = attribute.name().toString();
            if (name == QLatin1String("transform"))
                state.transform = parseTransform(attribute.value().toString()) * state.transform;
            else if (name == QLatin1String("style")) {
                const QStringList declarations = attribute.value().toString().split(QLatin1Char(';'));
                for (const QString &declaration : declarations) {
                    const int colon = declaration.indexOf(QLatin1Char(':'));
                    if (colon > 0)
                        applyProperty(declaration.left(colon).trimmed(),
                                      declaration.mid(colon + 1).trimmed());
                }
            } else {
                applyProperty(name, attribute.value().toString());
            }
        }
        return state;
    }

    // The root svg element's width/height + viewBox mapping, when present.
    QTransform viewBoxTransform(const QXmlStreamReader &xml)
    {
        const QXmlStreamAttributes attributes = xml.attributes();
        const QVector<qreal> viewBox =
                parseNumberList(attributes.value(QLatin1String("viewBox")).toString());
        if (viewBox.size() != 4 || viewBox.at(2) <= 0 || viewBox.at(3) <= 0)
            return QTransform();
        QTransform t = QTransform::fromTranslate(-viewBox.at(0), -viewBox.at(1));
        bool okW = false, okH = false;
        const qreal width = lengthToPixels(attributes.value(QLatin1String("width")).toString(), &okW);
        const qreal height = lengthToPixels(attributes.value(QLatin1String("height")).toString(), &okH);
        if (okW && okH && width > 0 && height > 0)
            t = t * QTransform::fromScale(width / viewBox.at(2), height / viewBox.at(3));
        return t;
    }

    qreal attributeValue(const QXmlStreamReader &xml, const char *name, qreal fallback = 0.0)
    {
        bool ok = false;
        const qreal value = xml.attributes().value(QLatin1String(name)).toString().toDouble(&ok);
        return ok ? value : fallback;
    }

    // True when the transform keeps rectangles/ellipses axis-aligned, so
    // the native FCD primitive can be used instead of a sampled outline.
    static bool axisPreserving(const QTransform &t)
    {
        return qFuzzyIsNull(t.m12()) && qFuzzyIsNull(t.m21());
    }

    Layer *layerForShape(const SvgState &state)
    {
        // The outline color wins when both are set - a filled FCD shape
        // draws fill and outline in the layer's one color anyway.
        return m_layers.layerFor(state.stroke.isValid() ? state.stroke : state.fill);
    }

    void addPrimitive(GraphicsPrimitive *primitive, const SvgState &state)
    {
        if (Layer *layer = layerForShape(state))
            primitive->setLayer(layer);
        m_sheet->addPrimitive(primitive);
        sawAnyShape = true;
    }

    // Emits one already-mapped (scene-space) painter path per the "best
    // primitive" rules described in the header.
    void emitSubpath(const Subpath &sub, const QTransform &transform, const SvgState &state)
    {
        const bool filled = state.fill.isValid() && sub.closed;
        if (!sub.curved) {
            const QList<QPolygonF> polygons = transform.map(sub.path).toSubpathPolygons();
            if (polygons.isEmpty())
                return;
            QPolygonF points = polygons.first();
            if (points.size() >= 2 && points.first() == points.last())
                points.removeLast();
            if (sub.closed && points.size() >= 3) {
                auto *polygon = new PrimitivePolygon();
                for (const QPointF &point : points)
                    polygon->appendVertex(point);
                polygon->setIsFilled(filled);
                addPrimitive(polygon, state);
            } else {
                for (int i = 0; i + 1 < points.size(); ++i) {
                    auto *line = new PrimitiveLine();
                    line->setControlPoint(0, points.at(i));
                    line->setControlPoint(1, points.at(i + 1));
                    addPrimitive(line, state);
                }
            }
            return;
        }
        if (!sub.closed && sub.segmentCount == 1 && sub.cubicCount == 1) {
            auto *bezier = new PrimitiveBezier();
            for (int i = 0; i < 4; ++i)
                bezier->setControlPoint(i, transform.map(sub.cubic[i]));
            addPrimitive(bezier, state);
            return;
        }
        // General curved subpath: an interpolating complex curve through
        // densely sampled points - the same representation BooleanOps picks
        // for its smooth results.
        const QVector<QPointF> sampled = BooleanOps::sampledVertices(transform.map(sub.path));
        if (sampled.size() < 2)
            return;
        auto *curve = new PrimitiveComplexCurve();
        for (const QPointF &point : sampled)
            curve->appendVertex(point);
        curve->setClosed(sub.closed);
        curve->setIsFilled(filled);
        addPrimitive(curve, state);
    }

    // Returns true when `tag` was a shape element this importer handles
    // (the element is consumed either way in that case).
    bool handleShape(const QString &tag, QXmlStreamReader &xml, const SvgState &state)
    {
        const QTransform &t = state.transform;
        if (tag == QLatin1String("line")) {
            auto *line = new PrimitiveLine();
            line->setControlPoint(0, t.map(QPointF(attributeValue(xml, "x1"), attributeValue(xml, "y1"))));
            line->setControlPoint(1, t.map(QPointF(attributeValue(xml, "x2"), attributeValue(xml, "y2"))));
            addPrimitive(line, state);
            xml.skipCurrentElement();
            return true;
        }
        if (tag == QLatin1String("rect")) {
            const QRectF rect(attributeValue(xml, "x"), attributeValue(xml, "y"),
                              attributeValue(xml, "width"), attributeValue(xml, "height"));
            xml.skipCurrentElement();
            if (rect.isEmpty())
                return true;
            const bool filled = state.fill.isValid();
            if (axisPreserving(t)) {
                auto *rectangle = new PrimitiveRectangle();
                rectangle->setControlPoint(0, t.map(rect.topLeft()));
                rectangle->setControlPoint(1, t.map(rect.bottomRight()));
                rectangle->setIsFilled(filled);
                addPrimitive(rectangle, state);
            } else {
                auto *polygon = new PrimitivePolygon();
                polygon->appendVertex(t.map(rect.topLeft()));
                polygon->appendVertex(t.map(rect.topRight()));
                polygon->appendVertex(t.map(rect.bottomRight()));
                polygon->appendVertex(t.map(rect.bottomLeft()));
                polygon->setIsFilled(filled);
                addPrimitive(polygon, state);
            }
            return true;
        }
        if (tag == QLatin1String("circle") || tag == QLatin1String("ellipse")) {
            const qreal cx = attributeValue(xml, "cx");
            const qreal cy = attributeValue(xml, "cy");
            const qreal rx = tag == QLatin1String("circle")
                    ? attributeValue(xml, "r") : attributeValue(xml, "rx");
            const qreal ry = tag == QLatin1String("circle")
                    ? attributeValue(xml, "r") : attributeValue(xml, "ry");
            xml.skipCurrentElement();
            if (rx <= 0 || ry <= 0)
                return true;
            const QRectF box(cx - rx, cy - ry, rx * 2, ry * 2);
            const bool filled = state.fill.isValid();
            if (axisPreserving(t)) {
                auto *ellipse = new PrimitiveEllipse();
                ellipse->setControlPoint(0, t.map(box.topLeft()));
                ellipse->setControlPoint(1, t.map(box.bottomRight()));
                ellipse->setIsFilled(filled);
                addPrimitive(ellipse, state);
            } else {
                Subpath sub;
                sub.path.addEllipse(box);
                sub.closed = true;
                sub.curved = true;
                sub.segmentCount = 4;
                emitSubpath(sub, t, state);
            }
            return true;
        }
        if (tag == QLatin1String("polyline") || tag == QLatin1String("polygon")) {
            const QVector<qreal> numbers =
                    parseNumberList(xml.attributes().value(QLatin1String("points")).toString());
            xml.skipCurrentElement();
            QVector<QPointF> points;
            for (int i = 0; i + 1 < numbers.size(); i += 2)
                points.append(t.map(QPointF(numbers.at(i), numbers.at(i + 1))));
            if (points.size() < 2)
                return true;
            if (tag == QLatin1String("polygon") && points.size() >= 3) {
                auto *polygon = new PrimitivePolygon();
                for (const QPointF &point : points)
                    polygon->appendVertex(point);
                polygon->setIsFilled(state.fill.isValid());
                addPrimitive(polygon, state);
            } else {
                for (int i = 0; i + 1 < points.size(); ++i) {
                    auto *line = new PrimitiveLine();
                    line->setControlPoint(0, points.at(i));
                    line->setControlPoint(1, points.at(i + 1));
                    addPrimitive(line, state);
                }
            }
            return true;
        }
        if (tag == QLatin1String("path")) {
            const QList<Subpath> subpaths =
                    parsePathData(xml.attributes().value(QLatin1String("d")).toString());
            xml.skipCurrentElement();
            for (const Subpath &sub : subpaths)
                emitSubpath(sub, t, state);
            return true;
        }
        if (tag == QLatin1String("text")) {
            const qreal x = attributeValue(xml, "x");
            const qreal y = attributeValue(xml, "y");
            qreal fontSize = 16.0; // the CSS default
            const QString fontAttribute =
                    xml.attributes().value(QLatin1String("font-size")).toString();
            if (!fontAttribute.isEmpty()) {
                bool ok = false;
                const qreal parsed = lengthToPixels(fontAttribute, &ok);
                if (ok && parsed > 0)
                    fontSize = parsed;
            }
            // Everything inside (tspans included) collapsed to plain text.
            const QString content = xml.readElementText(QXmlStreamReader::IncludeChildElements)
                    .simplified();
            if (content.isEmpty())
                return true;
            if (!axisPreserving(t)) {
                countSkipped(QStringLiteral("text (rotated)"));
                return true;
            }
            auto *text = new PrimitiveText();
            text->setText(content);
            const qreal size = qMax<qreal>(1.0, fontSize * std::abs(t.m22()));
            // SVG's y is the baseline; TY's anchor is the text box's top.
            text->setControlPoint(0, t.map(QPointF(x, y)) - QPointF(0, size));
            text->setSize(qMax(1, qRound(size)), qMax(1, qRound(size * 0.75)));
            SvgState textState = state;
            if (!textState.stroke.isValid() && textState.fill.isValid())
                textState.stroke = textState.fill; // text is painted by fill
            addPrimitive(text, textState);
            return true;
        }
        return false;
    }

    void countSkipped(const QString &what)
    {
        ++m_skipped[what];
    }

    void flushWarnings()
    {
        if (!m_warnings)
            return;
        for (auto it = m_skipped.constBegin(); it != m_skipped.constEnd(); ++it)
            m_warnings->append(QStringLiteral("%1 unsupported \"%2\" element(s)")
                                       .arg(it.value()).arg(it.key()));
        if (m_layers.overflowCount > 0)
            m_warnings->append(QStringLiteral(
                    "%1 element(s) exceeded the 16 layer slots and kept layer 0's color")
                    .arg(m_layers.overflowCount));
    }

    Sheet *m_sheet;
    QStringList *m_warnings;
    LayerAssigner m_layers;
    QHash<QString, int> m_skipped;
};

} // namespace

namespace SvgReader {

void read(const QString &text, Sheet *sheet, QStringList *warnings)
{
    sheet->clearPrimitives();
    Importer importer(sheet, warnings);
    importer.run(text);
    // Any renamed/recolored layer slots must reach the layer widgets.
    LayerList::getInstance().update();
}

bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage,
              QStringList *warnings)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = file.errorString();
        return false;
    }
    QTextStream stream(&file);
    const QString text = stream.readAll();
    // A cheap sanity check so a mis-picked binary file fails cleanly
    // instead of silently producing an empty drawing.
    if (!text.contains(QLatin1String("<svg"), Qt::CaseInsensitive)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("not an SVG document");
        return false;
    }
    read(text, sheet, warnings);
    return true;
}

} // namespace SvgReader
