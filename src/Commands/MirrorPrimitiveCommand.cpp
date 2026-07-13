/*
 * MirrorPrimitiveCommand.cpp
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

#include "MirrorPrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

MirrorPrimitiveCommand::MirrorPrimitiveCommand(GraphicsPrimitive *primitive, Qt::Orientation axis,
                                                const QPointF &pivot)
    : m_primitive(primitive), m_axis(axis), m_pivot(pivot)
{
    setText(QObject::tr("Mirror"));
}

void MirrorPrimitiveCommand::undo()
{
    m_primitive->mirror(m_axis, m_pivot);
}

void MirrorPrimitiveCommand::redo()
{
    m_primitive->mirror(m_axis, m_pivot);
}
