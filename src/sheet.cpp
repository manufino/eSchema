/*
 * sheet.cpp
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

#include "Sheet.h"
#include "SettingsManager.h"
#include "PrimitivePad.h"
#include <utility>

namespace {
// Falls back to the compiled-in spec default (matches
// GraphicsPrimitive::effectiveLineWidth()'s own fallback) rather than the 0
// QSettings::value() would return for a settings file saved before the
// "line_width" option existed.
qreal defaultLineWidthSetting()
{
    const qreal value = SettingsManager::getInstance().loadSetting("line_width").toDouble();
    return value > 0 ? value : 0.5;
}
}

Sheet::Sheet(QObject *parent) :
    QGraphicsScene(parent)
{
    m_lineWidth = defaultLineWidthSetting();
}

void Sheet::addPrimitive(GraphicsPrimitive *primitive)
{
    if (m_primitives.contains(primitive))
        return; // already added - see the idempotency note in Sheet.h
    addItem(primitive);
    m_primitives.append(primitive);
}

void Sheet::takePrimitive(GraphicsPrimitive *primitive)
{
    if (!m_primitives.contains(primitive))
        return; // already removed - see the idempotency note in Sheet.h
    removeItem(primitive);
    m_primitives.removeOne(primitive);
}

void Sheet::removePrimitive(GraphicsPrimitive *primitive)
{
    takePrimitive(primitive);
    delete primitive;
}

void Sheet::clearPrimitives()
{
    // QGraphicsScene::clear() deletes every item it still owns, which covers all
    // primitives here since they were all added via addItem() in addPrimitive().
    clear();
    m_primitives.clear();
    m_undoStack.clear();
    m_connectionDiameter = 2.0;
    m_lineWidth = defaultLineWidthSetting();
    m_lineWidthCircles = 0.35;
}

void Sheet::drawForeground(QPainter *painter, const QRectF &)
{
    for (GraphicsPrimitive *primitive : std::as_const(m_primitives)) {
        if (auto *pad = dynamic_cast<PrimitivePad *>(primitive)) {
            if (pad->isVisible())
                pad->paintHole(painter);
        }
    }
}
