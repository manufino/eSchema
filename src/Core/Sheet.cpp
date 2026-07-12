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
#include "LayerList.h"
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
// Same fallback pattern, for "connection_diameter" (matches
// PrimitiveConnection::effectiveDiameter()'s own fallback).
qreal defaultConnectionDiameterSetting()
{
    const qreal value = SettingsManager::getInstance().loadSetting("connection_diameter").toDouble();
    return value > 0 ? value : 2.0;
}
}

Sheet::Sheet(QObject *parent) :
    QGraphicsScene(parent)
{
    m_lineWidth = defaultLineWidthSetting();
    m_connectionDiameter = defaultConnectionDiameterSetting();

    connect(&LayerList::getInstance(), &LayerList::layerAppearanceChanged,
            this, &Sheet::refreshLayerAppearance);
}

void Sheet::addPrimitive(GraphicsPrimitive *primitive)
{
    if (m_primitives.contains(primitive))
        return; // already added - see the idempotency note in Sheet.h
    addItem(primitive);
    // A primitive can be added onto an already-locked/hidden layer (e.g.
    // reloading a file with "FJC K" lines, or a macro body parsed after a
    // lock toggle) - make sure it starts in the right on-screen state.
    primitive->syncLayerAppearance();
    m_primitives.append(primitive);
    emit primitivesChanged();
}

void Sheet::refreshLayerAppearance()
{
    for (GraphicsPrimitive *primitive : std::as_const(m_primitives))
        primitive->syncLayerAppearance();
    update();
}

void Sheet::takePrimitive(GraphicsPrimitive *primitive)
{
    if (!m_primitives.contains(primitive))
        return; // already removed - see the idempotency note in Sheet.h
    removeItem(primitive);
    m_primitives.removeOne(primitive);
    emit primitivesChanged();
}

void Sheet::removePrimitive(GraphicsPrimitive *primitive)
{
    takePrimitive(primitive);
    delete primitive;
}

void Sheet::clearPrimitives()
{
    // Drops the selection first so any resize-handle items (owned by
    // SelectionHandleController, outside m_primitives) get torn down through the
    // normal selectionChanged -> clearHandles() path before clear() below deletes
    // their target primitive out from under them - otherwise they're left dangling.
    clearSelection();

    // Must run before m_primitives.clear(): CreatePrimitiveCommand/
    // DeletePrimitiveCommand destructors decide whether they own their primitive
    // by checking whether it's still in m_primitives. Clearing that list first
    // would make every command wrongly conclude it owns (and must delete) a
    // primitive that the QGraphicsScene::clear() below already destroyed,
    // double-freeing it.
    m_undoStack.clear();

    // QGraphicsScene::clear() deletes every item it still owns, which covers all
    // primitives here since they were all added via addItem() in addPrimitive().
    clear();
    m_primitives.clear();
    m_connectionDiameter = defaultConnectionDiameterSetting();
    m_lineWidth = defaultLineWidthSetting();
    m_lineWidthCircles = 0.35;

    // Layer lock state (persisted per-file as "FJC K", see FidoCadWriter) is
    // process-wide (LayerList is a singleton, not per-Sheet) - reset it here
    // for the same reason as the document config above, so a lock from a
    // previously open document can't leak into whatever gets loaded/created
    // next. Covers every clearPrimitives() caller (New Drawing,
    // FidoCadReader::read(), DxfReader's imports) in one place.
    if (QList<Layer *> *layers = LayerList::getInstance().getList()) {
        for (Layer *layer : std::as_const(*layers))
            layer->setLocked(false);
    }

    emit primitivesChanged();
}

void Sheet::drawForeground(QPainter *painter, const QRectF &)
{
    for (GraphicsPrimitive *primitive : std::as_const(m_primitives)) {
        if (auto *pad = dynamic_cast<PrimitivePad *>(primitive)) {
            if (pad->isVisible() && pad->isOnCanvas())
                pad->paintHole(painter);
        }
    }
}
