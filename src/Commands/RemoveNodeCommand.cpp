/*
 * RemoveNodeCommand.cpp
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

#include "RemoveNodeCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

RemoveNodeCommand::RemoveNodeCommand(GraphicsPrimitive *primitive, int index)
    : m_primitive(primitive), m_index(index), m_point(primitive->controlPoint(index))
{
    setText(QObject::tr("Rimuovi nodo"));
}

void RemoveNodeCommand::undo()
{
    m_primitive->insertControlPoint(m_index, m_point);
}

void RemoveNodeCommand::redo()
{
    m_primitive->removeControlPointAt(m_index);
}
