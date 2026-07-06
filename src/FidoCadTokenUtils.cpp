/*
 * FidoCadTokenUtils.cpp
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

#include "FidoCadTokenUtils.h"
#include <cmath>
#include <algorithm>

namespace FidoCadTokenUtils {

QString roundIntelligently(qreal value)
{
    // Round to 3 decimals first so that e.g. 12.1249999999 (float error) prints
    // as "12.125", not "12.1249".
    const qreal rounded = std::round(value * 1000.0) / 1000.0;

    if (rounded == std::floor(rounded))
        return QString::number(qlonglong(rounded));

    QString text = QString::number(rounded, 'f', 3);
    // Trim trailing zeros, then a trailing bare decimal point if that's all
    // that's left.
    while (text.endsWith('0'))
        text.chop(1);
    if (text.endsWith('.'))
        text.chop(1);
    return text;
}

QString encodeFontName(const QString &fontName)
{
    if (fontName.isEmpty() || fontName == QStringLiteral("Courier New"))
        return QStringLiteral("*");
    QString encoded = fontName;
    encoded.replace(QLatin1Char(' '), QStringLiteral("++"));
    return encoded;
}

QString decodeFontName(const QString &token)
{
    if (token == QStringLiteral("*"))
        return QStringLiteral("Courier New");
    QString decoded = token;
    decoded.replace(QStringLiteral("++"), QStringLiteral(" "));
    return decoded;
}

qreal clampCoordinate(qreal value)
{
    return std::clamp(value, qreal(0), qreal(1000000));
}

int clampLayer(int layer)
{
    return std::clamp(layer, 0, 15);
}

int clampDashStyle(int dashStyle)
{
    return std::clamp(dashStyle, 0, 4);
}

Qt::PenStyle dashStyleToPenStyle(int dashStyle)
{
    switch (clampDashStyle(dashStyle)) {
    case 0: return Qt::SolidLine;
    case 1: return Qt::DashLine;
    case 2: return Qt::DotLine;
    case 3: return Qt::DashDotLine;
    case 4: return Qt::DashDotDotLine;
    }
    return Qt::SolidLine;
}

int penStyleToDashStyle(Qt::PenStyle style)
{
    switch (style) {
    case Qt::SolidLine: return 0;
    case Qt::DashLine: return 1;
    case Qt::DotLine: return 2;
    case Qt::DashDotLine: return 3;
    case Qt::DashDotDotLine: return 4;
    default: return 0;
    }
}

} // namespace FidoCadTokenUtils
