/*
 * MovePrimitiveCommand.h
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

#ifndef MOVEPRIMITIVECOMMAND_H
#define MOVEPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QVector>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for dragging a primitive's body (Select tool). Stores absolute
// control-point snapshots from before/after the drag rather than a relative
// delta: the drag already applied the "after" state live for visual feedback,
// so redo() must be safe to call again on push() without moving it twice.
class MovePrimitiveCommand : public QUndoCommand
{
public:
    // `before`/`after` are full control-point snapshots taken by the caller
    // (SelectionHandleController) around the drag; the primitive is borrowed.
    MovePrimitiveCommand(GraphicsPrimitive *primitive, const QVector<QPointF> &before,
                          const QVector<QPointF> &after);

    // Restores the pre-drag control points via setControlPoints().
    void undo() override;
    // Applies the post-drag control points; harmless on the automatic call at
    // push() time since the drag already left the primitive in that state.
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    QVector<QPointF> m_before;
    QVector<QPointF> m_after;
};

#endif // MOVEPRIMITIVECOMMAND_H
