/*
 * CreatePrimitiveCommand.cpp
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

#include "CreatePrimitiveCommand.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include <QObject>

CreatePrimitiveCommand::CreatePrimitiveCommand(Sheet *sheet, GraphicsPrimitive *primitive)
    : m_sheet(sheet), m_primitive(primitive)
{
    setText(QObject::tr("Create"));
}

CreatePrimitiveCommand::~CreatePrimitiveCommand()
{
    // If undo() was the last thing applied, the sheet no longer owns the
    // primitive - we do, and must clean it up ourselves.
    if (!m_sheet->primitives().contains(m_primitive))
        delete m_primitive;
}

void CreatePrimitiveCommand::undo()
{
    m_sheet->takePrimitive(m_primitive);
}

void CreatePrimitiveCommand::redo()
{
    m_sheet->addPrimitive(m_primitive);
}
