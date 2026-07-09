/*
 * CreatePrimitiveCommand.h
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

#ifndef CREATEPRIMITIVECOMMAND_H
#define CREATEPRIMITIVECOMMAND_H

#include <QUndoCommand>

class Sheet;
class GraphicsPrimitive;

// Undo/redo for finishing a drawing-tool placement. The primitive is already
// added to the sheet by the time this command is constructed and pushed (see
// PrimitivePlacementController::finishPlacement()) - Sheet::addPrimitive() is
// idempotent, so QUndoStack::push()'s automatic redo() call is a harmless
// no-op the first time.
//
// Ownership toggles between the Sheet and this command: undo() hands the
// primitive to takePrimitive() (removes it from the scene without deleting
// it), so this command becomes the sole owner until either redo() gives it
// back or this command itself is destroyed while still holding it.
class CreatePrimitiveCommand : public QUndoCommand
{
public:
    CreatePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive);
    ~CreatePrimitiveCommand() override;

    void undo() override;
    void redo() override;

private:
    Sheet *m_sheet;
    GraphicsPrimitive *m_primitive;
};

#endif // CREATEPRIMITIVECOMMAND_H
