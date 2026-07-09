/*
 * FidoCadTokenUtils.h
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

#ifndef FIDOCADTOKENUTILS_H
#define FIDOCADTOKENUTILS_H

#include <QString>
#include <Qt>

// Shared low-level helpers used by both FidoCadReader and FidoCadWriter, so the
// numeric formatting / clamping rules from FIDOSPECS.md are implemented exactly
// once instead of being duplicated on the read and write sides.
namespace FidoCadTokenUtils {

// Formats a coordinate/size value the way FidoCadJ does: integral values print
// without a decimal point, otherwise up to 3 decimals with trailing zeros (and a
// trailing bare decimal point) trimmed. See FIDOSPECS.md 2.3 / 9.6.
QString roundIntelligently(qreal value);

// Font-name field encoding: "*" means the default font (Courier New); a literal
// space inside a font name is escaped as "++" (space is the token separator).
QString encodeFontName(const QString &fontName);
QString decodeFontName(const QString &token);

// Coordinates are clamped to [0, 1 000 000] and never negative (FIDOSPECS.md 3).
qreal clampCoordinate(qreal value);

// Layer indices are clamped to [0, 15]; anything out of range (or non-numeric,
// handled by the caller before calling this) coerces to layer 0.
int clampLayer(int layer);

// FCJ dash-style index, clamped to the 5 styles FidoCadJ defines (0-4).
int clampDashStyle(int dashStyle);

// FidoCadJ's 5 FCJ dash styles map 1:1 onto the 5 real line styles already
// offered by PenStyleComboBox (Qt::SolidLine..Qt::DashDotDotLine). Qt has only
// one "dotted" pattern, so FCD's "fine dotted" (index 2) and "dotted" (index 3)
// both funnel through Qt::DotLine/Qt::DashDotLine respectively as the closest
// available approximation - exact dash geometry is a rendering nicety, not part
// of the document model.
Qt::PenStyle dashStyleToPenStyle(int dashStyle);
int penStyleToDashStyle(Qt::PenStyle style);

} // namespace FidoCadTokenUtils

#endif // FIDOCADTOKENUTILS_H
