/*
 * SelectionHandleController.cpp
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

#include "SelectionHandleController.h"
#include "Sheet.h"
#include "GraphicsPrimitive.h"
#include "PrimitiveHandleItem.h"
#include <utility>

SelectionHandleController::SelectionHandleController(Sheet *sheet, QObject *parent)
    : QObject(parent), m_sheet(sheet)
{
    connect(sheet, &QGraphicsScene::selectionChanged, this, &SelectionHandleController::onSelectionChanged);
    connect(sheet, &QGraphicsScene::changed, this, &SelectionHandleController::refreshHandlePositions);
}

void SelectionHandleController::onSelectionChanged()
{
    clearHandles();

    const QList<QGraphicsItem *> selected = m_sheet->selectedItems();
    if (selected.size() != 1)
        return;

    auto *primitive = dynamic_cast<GraphicsPrimitive *>(selected.first());
    if (!primitive)
        return;

    const int count = primitive->controlPointCount();
    for (int i = 0; i < count; ++i) {
        auto *handle = new PrimitiveHandleItem(primitive, i);
        m_sheet->addItem(handle);
        m_handles.append(handle);
    }
}

void SelectionHandleController::refreshHandlePositions()
{
    for (PrimitiveHandleItem *handle : std::as_const(m_handles))
        handle->setPos(handle->target()->controlPoint(handle->controlPointIndex()));
}

void SelectionHandleController::clearHandles()
{
    for (PrimitiveHandleItem *handle : std::as_const(m_handles)) {
        m_sheet->removeItem(handle);
        delete handle;
    }
    m_handles.clear();
}
