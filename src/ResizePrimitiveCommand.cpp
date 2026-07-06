/*
 * ResizePrimitiveCommand.cpp
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

#include "ResizePrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

ResizePrimitiveCommand::ResizePrimitiveCommand(GraphicsPrimitive *primitive, int controlPointIndex,
                                                const QPointF &before, const QPointF &after)
    : m_primitive(primitive), m_index(controlPointIndex), m_before(before), m_after(after)
{
    setText(QObject::tr("Ridimensiona"));
}

void ResizePrimitiveCommand::undo()
{
    m_primitive->setControlPoint(m_index, m_before);
}

void ResizePrimitiveCommand::redo()
{
    m_primitive->setControlPoint(m_index, m_after);
}
