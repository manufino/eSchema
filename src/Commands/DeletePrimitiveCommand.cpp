/*
 * DeletePrimitiveCommand.cpp
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

#include "DeletePrimitiveCommand.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include <QObject>

DeletePrimitiveCommand::DeletePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive)
    : m_sheet(sheet), m_primitive(primitive)
{
    setText(QObject::tr("Delete"));
}

DeletePrimitiveCommand::~DeletePrimitiveCommand()
{
    // Only if redo() was the last thing applied *to this command*: the
    // explicit flag (not a Sheet-membership test) keeps a coexisting
    // Create command for the same primitive from also concluding it owns
    // it - see CreatePrimitiveCommand's class comment.
    if (m_owns)
        delete m_primitive;
}

void DeletePrimitiveCommand::undo()
{
    m_sheet->addPrimitive(m_primitive);
    m_owns = false;
}

void DeletePrimitiveCommand::redo()
{
    m_sheet->takePrimitive(m_primitive);
    m_owns = true;
}
