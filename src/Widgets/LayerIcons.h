/*
 * LayerIcons.h
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

#ifndef LAYERICONS_H
#define LAYERICONS_H

#include <QPixmap>

// eSchema has no lock icon assets anywhere (resources/remix has an eye set
// but nothing padlock-shaped) - rather than vendoring new binary icon files,
// this renders a simple two-state padlock glyph directly with QPainter,
// matching the visual weight (simple black outline, transparent background)
// of the existing Remix Icon "line" style icons.
namespace LayerIcons {

// A closed (locked) or open-shackle (unlocked) padlock outline, size x size,
// transparent background - same 24x24 resolution as the existing eye/
// bookmark icons (LayerVisibilityButton), so it sits at the same visual
// weight next to them.
QPixmap renderLockIcon(bool locked, int size = 24);

} // namespace LayerIcons

#endif // LAYERICONS_H
