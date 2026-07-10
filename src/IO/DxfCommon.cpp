/*
 * DxfCommon.cpp
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

#include "DxfCommon.h"

#include <QtMath>
#include <cmath>

namespace DxfCommon {

namespace {

QVector<QColor> buildAciPalette()
{
    QVector<QColor> palette(256);

    // 1-9: AutoCAD's fixed standard colors.
    palette[1] = QColor(255, 0, 0);     // red
    palette[2] = QColor(255, 255, 0);   // yellow
    palette[3] = QColor(0, 255, 0);     // green
    palette[4] = QColor(0, 255, 255);   // cyan
    palette[5] = QColor(0, 0, 255);     // blue
    palette[6] = QColor(255, 0, 255);   // magenta
    palette[7] = QColor(255, 255, 255); // white/black (background-dependent)
    palette[8] = QColor(65, 65, 65);    // dark gray
    palette[9] = QColor(127, 127, 127); // light gray

    // 10-249: 24 hues x 10 shades, brightness/saturation tapering with shade
    // index - procedurally approximates AutoCAD's real (hand-tuned, no
    // closed-form) mid-range table.
    for (int h = 0; h < 24; ++h) {
        const qreal hue = (h * 15.0) / 360.0;
        for (int s = 0; s < 10; ++s) {
            const int index = 10 + h * 10 + s;
            const qreal value = qBound(0.15, 1.0 - s * 0.08, 1.0);
            const qreal saturation = s < 5 ? 1.0 : qBound(0.25, 1.0 - (s - 4) * 0.15, 1.0);
            palette[index] = QColor::fromHsvF(hue, saturation, value);
        }
    }

    // 250-255: grayscale ramp, dark to light.
    const int grays[6] = {51, 91, 132, 173, 214, 255};
    for (int i = 0; i < 6; ++i)
        palette[250 + i] = QColor(grays[i], grays[i], grays[i]);

    return palette;
}

const QVector<QColor> &aciPalette()
{
    static const QVector<QColor> palette = buildAciPalette();
    return palette;
}

} // namespace

QVector<GroupPair> tokenizePairs(const QString &text)
{
    QVector<GroupPair> pairs;

    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const QStringList lines = normalized.split(QLatin1Char('\n'));

    for (int i = 0; i + 1 < lines.size(); i += 2) {
        bool ok = false;
        const int code = lines.at(i).trimmed().toInt(&ok);
        if (!ok)
            continue; // malformed group-code line - skip just this pair
        pairs.append({code, lines.at(i + 1).trimmed()});
    }

    return pairs;
}

void appendGroup(QStringList &lines, int code, const QString &value)
{
    lines << QString::number(code) << value;
}

void appendGroup(QStringList &lines, int code, int value)
{
    appendGroup(lines, code, QString::number(value));
}

namespace {

// DXF's floating-point group codes (10-59, 140-147, 210-239, ...) must be
// written with an explicit decimal point - unlike FidoCadTokenUtils::
// roundIntelligently() (which deliberately prints whole numbers bare, e.g.
// "10" not "10.0"), a real AutoCAD-compatible parser expects every real
// value to look like one, and rejects a bare integer string there even
// though lenient readers (this app's own DxfReader, ezdxf, LibreCAD) parse
// it fine via a generic strtod(). Six decimals, trailing zeros trimmed but
// always leaving at least one digit after the point.
QString formatReal(qreal value)
{
    QString text = QString::number(value, 'f', 6);
    while (text.endsWith(QLatin1Char('0')) && !text.endsWith(QStringLiteral(".0")))
        text.chop(1);
    return text;
}

} // namespace

void appendGroup(QStringList &lines, int code, qreal value)
{
    appendGroup(lines, code, formatReal(value));
}

QColor aciToColor(int index)
{
    if (index < 1 || index > 255)
        return QColor(255, 255, 255);
    return aciPalette().at(index);
}

int colorToAci(const QColor &color)
{
    int best = 7;
    int bestDistance = std::numeric_limits<int>::max();
    const QVector<QColor> &palette = aciPalette();
    for (int i = 1; i <= 255; ++i) {
        const QColor &candidate = palette.at(i);
        const int dr = candidate.red() - color.red();
        const int dg = candidate.green() - color.green();
        const int db = candidate.blue() - color.blue();
        const int distance = dr * dr + dg * dg + db * db;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

QColor trueColorToQColor(qint32 value)
{
    return QColor((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

QVector<QPointF> sampleArc(const QPointF &center, qreal radius,
                            qreal startAngleDeg, qreal endAngleDeg, qreal stepDeg)
{
    QVector<QPointF> points;

    qreal sweep = endAngleDeg - startAngleDeg;
    while (sweep <= 0)
        sweep += 360.0;

    const int steps = qMax(1, int(std::ceil(sweep / stepDeg)));
    for (int i = 0; i <= steps; ++i) {
        const qreal angle = startAngleDeg + sweep * i / steps;
        const qreal rad = qDegreesToRadians(angle);
        points.append(center + QPointF(radius * std::cos(rad), radius * std::sin(rad)));
    }

    return points;
}

} // namespace DxfCommon
