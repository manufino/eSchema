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
// undo() hands it back via addPrimitive().
//
// Ownership is tracked with an explicit flag flipped in undo()/redo(),
// never inferred from Sheet membership - see CreatePrimitiveCommand's class
// comment for the double-free a membership test causes when a Create and a
// Delete command for the same primitive coexist on the stack.
class DeletePrimitiveCommand : public QUndoCommand
{
public:
    // Both pointers are borrowed at this point: `primitive` is still in
    // `sheet` (removal happens in the first redo()) and the sheet owns it.
    DeletePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive);
    // Deletes the primitive only when this command still owns it, i.e. the
    // deletion was never undone before the stack dropped the command.
    ~DeletePrimitiveCommand() override;

    // Puts the primitive back with Sheet::addPrimitive() and releases
    // ownership.
    void undo() override;
    // Detaches the primitive from the sheet via Sheet::takePrimitive() (no
    // deletion) and takes ownership of it.
    void redo() override;

private:
    Sheet *m_sheet;
    GraphicsPrimitive *m_primitive;
    // True only between a redo() (this command holds the detached
    // primitive) and the next undo()/destruction.
    bool m_owns = false;
};

#endif // DELETEPRIMITIVECOMMAND_H
