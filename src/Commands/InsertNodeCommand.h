/*
 * InsertNodeCommand.h
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

#ifndef INSERTNODECOMMAND_H
#define INSERTNODECOMMAND_H

#include <QUndoCommand>
#include <QPointF>

class GraphicsPrimitive;

// Undo/redo for the canvas right-click menu's add-node action (polygons and
// complex curves only - see GraphicsPrimitive::supportsNodeEditing()).
class InsertNodeCommand : public QUndoCommand
{
public:
    InsertNodeCommand(GraphicsPrimitive *primitive, int index, const QPointF &scenePos);

    void undo() override;
    void redo() override;

private:
    GraphicsPrimitive *m_primitive;
    int m_index;
    QPointF m_point;
};

#endif // INSERTNODECOMMAND_H
