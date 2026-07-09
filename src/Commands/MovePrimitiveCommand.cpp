/*
 * MovePrimitiveCommand.cpp
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

#include "MovePrimitiveCommand.h"
#include "GraphicsPrimitive.h"
#include <QObject>

MovePrimitiveCommand::MovePrimitiveCommand(GraphicsPrimitive *primitive, const QVector<QPointF> &before,
                                            const QVector<QPointF> &after)
    : m_primitive(primitive), m_before(before), m_after(after)
{
    setText(QObject::tr("Sposta"));
}

void MovePrimitiveCommand::undo()
{
    m_primitive->restoreControlPoints(m_before);
}

void MovePrimitiveCommand::redo()
{
    m_primitive->restoreControlPoints(m_after);
}
