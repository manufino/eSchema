/*
 * RemoveNodeCommand.h
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

#ifndef REMOVENODECOMMAND_H
#define REMOVENODECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for the canvas right-click menu's remove-node action (polygons
// and complex curves only - see GraphicsPrimitive::supportsNodeEditing()).
class RemoveNodeCommand : public QUndoCommand
{
public:
    // Captures the vertex's current position up front, before redo() (called
    // once by QUndoStack::push() right after construction) removes it.
    RemoveNodeCommand(GraphicsPrimitive *primitive, int index);

    // Re-inserts the removed vertex at its captured index/position via
    // GraphicsPrimitive::insertControlPoint().
    void undo() override;
    // Removes the vertex via GraphicsPrimitive::removeControlPoint().
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    int m_index;
    QPointF m_point;
};

#endif // REMOVENODECOMMAND_H
