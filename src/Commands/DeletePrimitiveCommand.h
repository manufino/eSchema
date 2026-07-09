/*
 * DeletePrimitiveCommand.h
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

#ifndef DELETEPRIMITIVECOMMAND_H
#define DELETEPRIMITIVECOMMAND_H

#include <QUndoCommand>

class Sheet;
class GraphicsPrimitive;

// Undo/redo for Edit > Delete. Mirrors CreatePrimitiveCommand: constructed
// while the primitive is still in the sheet, redo() (auto-called once by
// QUndoStack::push()) removes it via takePrimitive() without deleting it, and
// undo() hands it back via addPrimitive(). Whichever state the command isn't
// currently "holding" the primitive in when destroyed determines whether the
// destructor needs to delete it.
class DeletePrimitiveCommand : public QUndoCommand
{
public:
    DeletePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive);
    ~DeletePrimitiveCommand() override;

    void undo() override;
    void redo() override;

private:
    Sheet *m_sheet;
    GraphicsPrimitive *m_primitive;
};

#endif // DELETEPRIMITIVECOMMAND_H
