/*
 * EpsGenerator.cpp
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

#include "EpsGenerator.h"
#include "appversion.h"

#include <QFile>
#include <QImage>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QTextStream>

// Translates QPainter calls into PostScript. Advertises (nearly) every
// feature, exactly like QSvgGenerator's engine does: any feature NOT
// advertised makes QPainter emulate the drawing by rasterizing it into an
// image and handing that to drawImage() - which silently turns every line
// of the schematic into an embedded bitmap, defeating a vector export.
// Advertising the features instead means geometry arrives untransformed
// with the painter transform in the state; drawPath() applies it with a
// PostScript `concat`, so pen widths/dashes scale in the interpreter just
// as they would on screen. Rectangles, ellipses, polygons, lines and text
// (via the base drawTextItem()'s outline fallback) all funnel into
// drawPath(), so filling/stroking a path and blitting an image is still
// all this engine has to implement.
class EpsPaintEngine : public QPaintEngine
{
public:
    EpsPaintEngine()
        : QPaintEngine(QPaintEngine::PaintEngineFeatures(QPaintEngine::AllFeatures
                       & ~QPaintEngine::PatternBrush
                       & ~QPaintEngine::PerspectiveTransform
                       & ~QPaintEngine::ConicalGradientFill
                       & ~QPaintEngine::PorterDuff))
    {}

    void setOutput(const QString &fileName, const QSize &size, const QString &title)
    {
        m_fileName = fileName;
        m_size = size;
        m_title = title;
    }

    bool begin(QPaintDevice *pdev) override
    {
        Q_UNUSED(pdev);
        m_file.setFileName(m_fileName);
        if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        m_out.setDevice(&m_file);

        // PostScript's y axis points up with the origin at the bottom-left;
        // Qt's points down from the top-left. The translate+scale prologue
        // flips the page once so every coordinate can be written as-is.
        m_out << "%!PS-Adobe-3.0 EPSF-3.0\n"
              << "%%BoundingBox: 0 0 " << m_size.width() << ' ' << m_size.height() << '\n'
              << "%%Title: " << (m_title.isEmpty() ? QStringLiteral("eSchema drawing") : m_title) << '\n'
              << "%%Creator: eSchema " << APP_VERSION << '\n'
              << "%%Pages: 1\n"
              << "%%EndComments\n"
              << "save\n"
              << "0 " << m_size.height() << " translate\n"
              << "1 -1 scale\n"
              << "1 setlinecap 1 setlinejoin\n";
        return true;
    }

    bool end() override
    {
        m_out << "restore\n"
              << "showpage\n"
              << "%%EOF\n";
        m_out.flush();
        const bool ok = m_out.status() == QTextStream::Ok;
        m_file.close();
        return ok && m_file.error() == QFileDevice::NoError;
    }

    void updateState(const QPaintEngineState &state) override
    {
        if (state.state() & QPaintEngine::DirtyPen)
            m_pen = state.pen();
        if (state.state() & QPaintEngine::DirtyBrush)
            m_brush = state.brush();
        if (state.state() & QPaintEngine::DirtyTransform)
            m_transform = state.transform();
        // Clipping is ignored (EPS-side), same documented limitation as
        // QSvgGenerator - nothing in a schematic render clips anyway.
    }

    void drawPath(const QPainterPath &path) override
    {
        if (path.isEmpty())
            return;

        if (m_brush.style() != Qt::NoBrush) {
            m_out << "gsave\n";
            writeTransform();
            writeColor(m_brush.color());
            writePath(path);
            m_out << (path.fillRule() == Qt::OddEvenFill ? "eofill\n" : "fill\n");
            m_out << "grestore\n";
        }

        if (m_pen.style() != Qt::NoPen) {
            m_out << "gsave\n";
            writeTransform();
            writeColor(m_pen.color());
            // Both width and dash lengths are in logical units here - the
            // concat emitted above makes the interpreter scale them along
            // with the geometry, matching Qt's own non-cosmetic stroking.
            // A zero-width Qt pen means "thinnest possible" (cosmetic);
            // PostScript's own 0 setlinewidth has exactly the same meaning.
            m_out << formatNumber(qMax(0.0, m_pen.widthF())) << " setlinewidth\n";
            writeDashPattern();
            writePath(path);
            m_out << "stroke\n";
            m_out << "grestore\n";
        }
    }

    // Mandatory despite drawPath() above: QPaintEngine's two drawPolygon()
    // default implementations (QPointF/QPoint) delegate to each other, so
    // leaving both unimplemented recurses until the stack overflows the
    // moment anything draws a line or polygon.
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override
    {
        if (pointCount < 2)
            return;
        QPainterPath path;
        path.moveTo(points[0]);
        for (int i = 1; i < pointCount; ++i)
            path.lineTo(points[i]);
        if (mode == PolylineMode) {
            // An open polyline is stroke-only - the current brush must not
            // fill it as if it were a closed shape.
            const QBrush savedBrush = m_brush;
            m_brush = Qt::NoBrush;
            drawPath(path);
            m_brush = savedBrush;
        } else {
            path.closeSubpath();
            path.setFillRule(mode == OddEvenMode ? Qt::OddEvenFill : Qt::WindingFill);
            drawPath(path);
        }
    }

    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override
    {
        drawImage(r, pm.toImage(), sr, Qt::AutoColor);
    }

    void drawImage(const QRectF &r, const QImage &image, const QRectF &sr,
                   Qt::ImageConversionFlags flags) override
    {
        Q_UNUSED(flags);
        if (r.isEmpty() || sr.isEmpty() || image.isNull())
            return;

        // EPS has no alpha channel - flatten the (sub-)image onto white,
        // matching every other export path's white page background.
        const QImage sub = image.copy(sr.toAlignedRect());
        QImage flat(sub.size(), QImage::Format_RGB32);
        flat.fill(Qt::white);
        {
            QPainter compositor(&flat);
            compositor.drawImage(0, 0, sub);
        }

        const int width = flat.width();
        const int height = flat.height();

        m_out << "gsave\n";
        writeTransform();
        m_out << formatNumber(r.x()) << ' ' << formatNumber(r.y()) << " translate\n"
              << formatNumber(r.width()) << ' ' << formatNumber(r.height()) << " scale\n"
              << "/DeviceRGB setcolorspace\n"
              << "<< /ImageType 1 /Width " << width << " /Height " << height
              << " /BitsPerComponent 8 /Decode [0 1 0 1 0 1]\n"
              // The page's CTM is already y-flipped (see begin()), so the
              // unit square's y grows downward like the data's row order -
              // a plain row-major matrix keeps row 0 at the rect's top.
              << "   /ImageMatrix [" << width << " 0 0 " << height << " 0 0]\n"
              << "   /DataSource currentfile /ASCIIHexDecode filter >>\n"
              << "image\n";

        static const char hexDigits[] = "0123456789abcdef";
        QByteArray hex;
        hex.reserve(width * 6 + 1);
        for (int y = 0; y < height; ++y) {
            hex.clear();
            const QRgb *line = reinterpret_cast<const QRgb *>(flat.constScanLine(y));
            for (int x = 0; x < width; ++x) {
                const QRgb rgb = line[x];
                hex.append(hexDigits[qRed(rgb) >> 4]).append(hexDigits[qRed(rgb) & 0xf]);
                hex.append(hexDigits[qGreen(rgb) >> 4]).append(hexDigits[qGreen(rgb) & 0xf]);
                hex.append(hexDigits[qBlue(rgb) >> 4]).append(hexDigits[qBlue(rgb) & 0xf]);
            }
            m_out << hex << '\n';
        }
        m_out << ">\n"
              << "grestore\n";
    }

    Type type() const override { return QPaintEngine::User; }

private:
    // Fixed-point with a fixed locale ('.' decimal separator, no exponent) -
    // QTextStream's default double formatting is locale-independent but can
    // emit scientific notation, which PostScript does not parse.
    static QString formatNumber(qreal value)
    {
        return QString::number(value, 'f', 3);
    }

    // The painter transform as a PostScript CTM concat. Qt's mapping
    // x' = m11*x + m21*y + dx matches PS's x' = a*x + c*y + e with
    // [a b c d e f] = [m11 m12 m21 m22 dx dy].
    void writeTransform()
    {
        if (m_transform.isIdentity())
            return;
        m_out << '[' << formatNumber(m_transform.m11()) << ' '
              << formatNumber(m_transform.m12()) << ' '
              << formatNumber(m_transform.m21()) << ' '
              << formatNumber(m_transform.m22()) << ' '
              << formatNumber(m_transform.dx()) << ' '
              << formatNumber(m_transform.dy()) << "] concat\n";
    }

    void writeColor(const QColor &color)
    {
        m_out << formatNumber(color.redF()) << ' '
              << formatNumber(color.greenF()) << ' '
              << formatNumber(color.blueF()) << " setrgbcolor\n";
    }

    void writeDashPattern()
    {
        if (m_pen.style() == Qt::SolidLine || m_pen.style() == Qt::NoPen) {
            m_out << "[] 0 setdash\n";
            return;
        }
        // dashPattern() covers both the stock styles (Dash/Dot/...) and
        // custom patterns, expressed in pen-width units - PostScript wants
        // absolute user-space lengths.
        const qreal unit = m_pen.widthF() > 0 ? m_pen.widthF() : 1.0;
        m_out << '[';
        const QList<qreal> pattern = m_pen.dashPattern();
        for (int i = 0; i < pattern.size(); ++i) {
            if (i > 0)
                m_out << ' ';
            m_out << formatNumber(pattern.at(i) * unit);
        }
        m_out << "] 0 setdash\n";
    }

    void writePath(const QPainterPath &path)
    {
        m_out << "newpath\n";
        for (int i = 0; i < path.elementCount(); ++i) {
            const QPainterPath::Element &element = path.elementAt(i);
            switch (element.type) {
            case QPainterPath::MoveToElement:
                m_out << formatNumber(element.x) << ' ' << formatNumber(element.y)
                      << " moveto\n";
                break;
            case QPainterPath::LineToElement:
                m_out << formatNumber(element.x) << ' ' << formatNumber(element.y)
                      << " lineto\n";
                break;
            case QPainterPath::CurveToElement: {
                // A CurveTo element is followed by its two CurveToData ones.
                const QPainterPath::Element &c2 = path.elementAt(i + 1);
                const QPainterPath::Element &endPoint = path.elementAt(i + 2);
                m_out << formatNumber(element.x) << ' ' << formatNumber(element.y) << ' '
                      << formatNumber(c2.x) << ' ' << formatNumber(c2.y) << ' '
                      << formatNumber(endPoint.x) << ' ' << formatNumber(endPoint.y)
                      << " curveto\n";
                i += 2;
                break;
            }
            case QPainterPath::CurveToDataElement:
                break; // consumed by the CurveToElement case above
            }
        }
    }

    QString m_fileName;
    QSize m_size;
    QString m_title;
    QFile m_file;
    QTextStream m_out;
    QPen m_pen;
    QBrush m_brush;
    QTransform m_transform;
};

EpsGenerator::EpsGenerator()
    : m_engine(new EpsPaintEngine)
{
}

EpsGenerator::~EpsGenerator() = default;

void EpsGenerator::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}

void EpsGenerator::setSize(const QSize &size)
{
    m_size = size;
}

void EpsGenerator::setTitle(const QString &title)
{
    m_title = title;
}

QPaintEngine *EpsGenerator::paintEngine() const
{
    // Called by QPainter::begin(), i.e. only after the setters have run -
    // the right moment to hand the engine its output parameters.
    m_engine->setOutput(m_fileName, m_size, m_title);
    return m_engine.data();
}

int EpsGenerator::metric(PaintDeviceMetric metric) const
{
    switch (metric) {
    case PdmWidth:
        return m_size.width();
    case PdmHeight:
        return m_size.height();
    case PdmWidthMM:
        return qRound(m_size.width() * 25.4 / 72.0);
    case PdmHeightMM:
        return qRound(m_size.height() * 25.4 / 72.0);
    case PdmNumColors:
        return 0xffffff;
    case PdmDepth:
        return 24;
    case PdmDpiX:
    case PdmDpiY:
    case PdmPhysicalDpiX:
    case PdmPhysicalDpiY:
        return 72;
    case PdmDevicePixelRatio:
        return 1;
    case PdmDevicePixelRatioScaled:
        return qRound(devicePixelRatioFScale());
    default:
        return QPaintDevice::metric(metric);
    }
}
