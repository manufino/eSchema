/*
 * RotatePrimitiveCommand.cpp
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

#include "RotatePrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

RotatePrimitiveCommand::RotatePrimitiveCommand(GraphicsPrimitive *primitive, const QPointF &pivot)
    : m_primitive(primitive), m_pivot(pivot)
{
    setText(QObject::tr("Rotate"));
}

void RotatePrimitiveCommand::undo()
{
    // 3 more quarter-turns = 4 total from the pre-redo state = identity.
    m_primitive->rotate90(m_pivot);
    m_primitive->rotate90(m_pivot);
    m_primitive->rotate90(m_pivot);
}

void RotatePrimitiveCommand::redo()
{
    m_primitive->rotate90(m_pivot);
}
