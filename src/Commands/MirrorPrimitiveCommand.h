/*
 * MirrorPrimitiveCommand.h
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

#ifndef MIRRORPRIMITIVECOMMAND_H
#define MIRRORPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for Edit > Mirror. Unlike Move/Resize, this isn't applied live
// before the command is pushed - redo() (auto-called once by
// QUndoStack::push()) is the only place GraphicsPrimitive::mirror() is
// actually invoked. Mirroring twice around the same axis/pivot is an
// identity operation (for both the reflected points and any extra
// orientation/mirror field a subclass like PrimitiveMacro/PrimitiveText
// toggles), so undo() just calls mirror() again instead of needing its own
// separate "before" state.
class MirrorPrimitiveCommand : public QUndoCommand
{
public:
    // `axis` is the reflection axis orientation and `pivot` the scene point
    // it passes through; the primitive is borrowed (owned by its Sheet).
    MirrorPrimitiveCommand(GraphicsPrimitive *primitive, Qt::Orientation axis, const QPointF &pivot);

    // Mirrors again around the same axis/pivot - reflection is its own
    // inverse, so this restores the original state.
    void undo() override;
    // Applies GraphicsPrimitive::mirror(); this is the only place the mirror
    // actually happens (nothing was applied before the push).
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    Qt::Orientation m_axis;
    QPointF m_pivot;
};

#endif // MIRRORPRIMITIVECOMMAND_H
