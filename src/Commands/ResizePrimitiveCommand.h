/*
 * ResizePrimitiveCommand.h
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

#ifndef RESIZEPRIMITIVECOMMAND_H
#define RESIZEPRIMITIVECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for dragging a single resize handle. Like MovePrimitiveCommand,
// stores absolute before/after positions (the drag already applied "after"
// live), so redo() is safe to call again when the command is pushed.
class ResizePrimitiveCommand : public QUndoCommand
{
public:
    ResizePrimitiveCommand(GraphicsPrimitive *primitive, int controlPointIndex,
                            const QPointF &before, const QPointF &after);

    void undo() override;
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    int m_index;
    QPointF m_before;
    QPointF m_after;
};

#endif // RESIZEPRIMITIVECOMMAND_H
