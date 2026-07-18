/*
 * SvgReader.h
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

#ifndef SVGREADER_H
#define SVGREADER_H

#include <QString>
#include <QStringList>

class Sheet;

// Converts an SVG document into FCD primitives, mirroring DxfReader's shape
// and contract: per-element unsupported/malformed content is skipped (and
// summarized in `warnings`) rather than aborting the import.
//
// Supported: line, rect, circle, ellipse, polyline, polygon, path (the full
// `d` grammar - M/L/H/V/C/S/Q/T/A/Z, absolute and relative, arcs converted
// to cubics), basic text, and g/svg nesting with transform (translate/
// scale/rotate/matrix) and inherited stroke/fill (presentation attributes
// and inline style). Geometry maps as: straight-only shapes to polygons/
// lines, a lone cubic to a Bezier, anything curved to an interpolating
// complex curve through sampled points (same convention as BooleanOps'
// smooth results). Distinct stroke/fill colors claim global layer slots
// 1-15 (renamed "SVG #rrggbb" and recolored), exactly like DXF layers do;
// black stays on layer 0. Coordinates are scaled 200/96 (CSS px are 1/96",
// the FidoCadJ unit is 1/200"), so physical dimensions are preserved.
//
// Not supported (skipped with a warning): images, gradients/patterns
// (their fills fall back to the default color), clipping/masking, use/defs
// references, markers, filters.
namespace SvgReader {

// Clears `sheet` and populates it from `text`. `warnings`, if given,
// collects one human-readable line per kind of skipped content.
void read(const QString &text, Sheet *sheet, QStringList *warnings = nullptr);

// Reads from disk. Returns false and sets *errorMessage if the file can't
// be opened or isn't XML at all; per-element issues never fail the read,
// they only appear in *warnings.
bool readFile(const QString &filePath, Sheet *sheet, QString *errorMessage = nullptr,
              QStringList *warnings = nullptr);

} // namespace SvgReader

#endif // SVGREADER_H
