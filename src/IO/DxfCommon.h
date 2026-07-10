/*
 * DxfCommon.h
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

#ifndef DXFCOMMON_H
#define DXFCOMMON_H

#include <QVector>
#include <QColor>
#include <QPointF>

// Shared infrastructure for DxfReader/DxfWriter: the AutoCAD Color Index
// (ACI) palette and arc tessellation. Actual DXF group-code reading/writing
// is delegated entirely to the vendored libdxfrw (see
// third_party/libdxfrw/README-eschema.md) - no group-code handling lives
// here anymore.
namespace DxfCommon {

// Approximates the standard 256-entry AutoCAD Color Index (ACI) palette
// (group code 62): colors 1-9 (primaries) and 250-255 (grayscale ramp) are
// exact/well-known anchors; 10-249 are procedurally generated across 24 hues
// x 10 shades, since AutoCAD's official mid-range table has no closed-form
// definition and isn't reliably reproducible by hand - close enough for
// nearest-color round-tripping between arbitrary QColors and ACI indices.
// Index 0 (ByBlock) and 256 (ByLayer) are not real colors - callers never
// look those up here.
QColor aciToColor(int index);
// Nearest ACI index (1-255) to `color`, by RGB Euclidean distance.
int colorToAci(const QColor &color);

// DXF true-color group 420 encodes 0x00RRGGBB.
QColor trueColorToQColor(qint32 value);

// Samples points every `stepDeg` degrees along a circular arc, sweeping
// counter-clockwise from `startAngleDeg` to `endAngleDeg` (DXF's own
// convention), wrapping around through 360 when end <= start. Used to
// approximate ARC entities (and LWPOLYLINE/POLYLINE bulge segments) as a
// PrimitiveComplexCurve, since this app has no arc/pie primitive.
QVector<QPointF> sampleArc(const QPointF &center, qreal radius,
                            qreal startAngleDeg, qreal endAngleDeg, qreal stepDeg = 5.0);

} // namespace DxfCommon

#endif // DXFCOMMON_H
