/*
 * ObjectSnap.h
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

#ifndef OBJECTSNAP_H
#define OBJECTSNAP_H

#include <QList>
#include <QPointF>
#include <optional>

class Sheet;
class GraphicsPrimitive;

// Object snapping: the characteristic points of what's already drawn -
// endpoints/vertices, straight-segment midpoints, rectangle/ellipse centers
// (plus the ellipse's four quadrant points), and intersections between
// nearby straight segments - as click targets that take precedence over the
// plain grid. What LibreCAD/AutoCAD call "object snap"; the reference
// FidoCadJ editor has no equivalent.
namespace ObjectSnap {

// The candidate point nearest `scenePos` within `radius` (scene units)
// among `sheet`'s visible primitives, or nullopt when none qualifies.
// `excluded` primitives contribute no candidates - callers pass whatever is
// being placed or dragged right now, which would otherwise snap onto
// itself.
std::optional<QPointF> snapPoint(const Sheet *sheet, const QPointF &scenePos, qreal radius,
                                 const QList<const GraphicsPrimitive *> &excluded);

}

#endif // OBJECTSNAP_H
