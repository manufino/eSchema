/*
 * RotatePrimitiveCommand.h
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

#ifndef ROTATEPRIMITIVECOMMAND_H
#define ROTATEPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for Edit > Rotate (a fixed 90 degree turn). redo() applies one
// rotate90(); since 4 applications return to the original state (for both
// the rotated points and any extra orientation field a subclass like
// PrimitiveMacro/PrimitiveText advances mod 4/360), undo() applies 3 more,
// landing back where it started without needing a separate "before" snapshot.
class RotatePrimitiveCommand : public QUndoCommand
{
public:
    // `pivot` is the scene point the 90 degree turn revolves around; the
    // primitive is borrowed (owned by its Sheet).
    RotatePrimitiveCommand(GraphicsPrimitive *primitive, const QPointF &pivot);

    // Applies rotate90() three more times (270 degrees), completing the full
    // turn back to the original state.
    void undo() override;
    // Applies one GraphicsPrimitive::rotate90() around the pivot; this is the
    // only place the rotation actually happens.
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    QPointF m_pivot;
};

#endif // ROTATEPRIMITIVECOMMAND_H
