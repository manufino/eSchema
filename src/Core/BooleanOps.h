/*
 * BooleanOps.h
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

#ifndef BOOLEANOPS_H
#define BOOLEANOPS_H

#include <QList>
#include <QPointF>
#include <QVector>

class GraphicsPrimitive;
class QPainterPath;

// Boolean path operations (union/subtraction/intersection) between closed
// shapes, built on QPainterPath's own set operations. The results are always
// expressed with primitives the FCD format already has - PP/PV polygons and
// closed CP/CV complex curves - so a file containing them opens unchanged in
// the reference FidoCadJ editor too; no format extension involved.
namespace BooleanOps {

enum class Operation {
    Union,
    Subtraction,   // first operand (document order) minus all the others
    Intersection,
};

// Builds the primitives replacing `operands` under `operation` - not added
// to any scene yet, in fresh unselected state; the caller owns them and is
// expected to push them through the undo stack together with the operands'
// deletion. Layer, fill and dash style are copied from the first operand.
// Returns an empty list when the result region is empty (e.g. intersecting
// disjoint shapes) - the caller should then leave the selection untouched.
//
// Result representation, chosen per closed contour of the result region:
// - only straight edges: a PP/PV polygon with the exact vertices;
// - curved edges, `smoothResults` false: a polygon with the curves flattened;
// - curved edges, `smoothResults` true: a closed CP/CV complex curve through
//   points sampled along the contour - far fewer nodes for the same visual
//   quality, at the cost of slightly rounded corners (the FCD complex curve
//   is an interpolating spline with no sharp-corner support);
// - contours with holes: a single "keyhole" polygon (the hole outlines
//   connected to the outer one through zero-width seams), since the format
//   has no multi-contour path primitive; always flattened, because a spline
//   would smear the seams into visible artifacts.
QList<GraphicsPrimitive *> combine(const QList<GraphicsPrimitive *> &operands,
                                   Operation operation, bool smoothResults);

// Builds the polygon/closed-complex-curve primitives covering `path`'s fill
// region, styled after `styleSource` - the same per-contour representation
// rules combine() uses for its results (see its doc comment above; combine()
// itself delegates here). Also used by Edit > Shape > Offset outline, whose
// region comes from QPainterPathStroker instead of a set operation. Returns
// an empty list for an empty region.
QList<GraphicsPrimitive *> primitivesFromPath(const QPainterPath &path,
                                              GraphicsPrimitive *styleSource,
                                              bool smoothResults);

// The exact vertices of `path`'s (first) contour when it has only straight
// edges - no sampling loss at all. Also used by Edit > Convert to polygon.
QVector<QPointF> exactVertices(const QPainterPath &path);

// Vertices sampled along `path`, densely enough (see the .cpp's
// SmoothSampleStep) that either a polygon or an interpolating spline through
// them tracks the original outline faithfully. Shared by the boolean smooth
// results and Edit > Convert to polygon/complex curve.
QVector<QPointF> sampledVertices(const QPainterPath &path);

}

#endif // BOOLEANOPS_H
