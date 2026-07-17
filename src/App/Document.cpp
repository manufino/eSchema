/*
 * Document.cpp
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

#include "Document.h"
#include "Sheet.h"
#include "LayerList.h"
#include <QFileInfo>
#include <QUndoStack>

Document::Document(QObject *parent)
    : QObject(parent)
{
    addSheet();
}

Document::~Document()
{
    qDeleteAll(m_sheets);
}

void Document::setActiveSheetIndex(int index)
{
    if (index >= 0 && index < m_sheets.size())
        m_activeSheet = index;
}

Sheet *Document::addSheet()
{
    // Deliberately parentless: Sheet is a QGraphicsScene whose lifetime this
    // document manages explicitly in the destructor (the view referencing it
    // must be detached first - MainWindow handles the ordering).
    auto *sheet = new Sheet();
    m_sheets.append(sheet);
    return sheet;
}

QString Document::displayName() const
{
    if (!m_filePath.isEmpty())
        return QFileInfo(m_filePath).fileName();
    return m_untitledNumber > 0 ? tr("New drawing %1").arg(m_untitledNumber)
                                : tr("New drawing");
}

bool Document::isClean() const
{
    for (Sheet *sheet : m_sheets) {
        if (!const_cast<Sheet *>(sheet)->undoStack()->isClean())
            return false;
    }
    return true;
}

void Document::captureLayerState()
{
    QList<Layer *> *layers = LayerList::getInstance().getList();
    if (!layers)
        return;
    m_layerStates.clear();
    for (Layer *layer : std::as_const(*layers)) {
        LayerState state;
        state.name = layer->name();
        state.color = layer->color();
        state.visible = layer->isVisible();
        state.locked = layer->isLocked();
        state.master = layer->isMaster();
        m_layerStates.append(state);
    }
    m_hasLayerState = true;
}

void Document::applyLayerState()
{
    // A document that was never switched away from has no snapshot yet: it
    // is simply "whatever the global list currently holds" (exactly the
    // pre-multidocument behavior), so there is nothing to restore.
    if (!m_hasLayerState)
        return;

    LayerList &layerList = LayerList::getInstance();
    QList<Layer *> *layers = layerList.getList();
    if (!layers)
        return;

    // Grow the shared roster if this document knew more layers than it
    // currently holds. Never shrink it: another open document may still
    // reference the extra Layer objects, and stale trailing layers are
    // harmless to a document whose primitives don't use them.
    while (layers->size() < m_layerStates.size())
        layerList.addLayer(m_layerStates.at(layers->size()).name,
                           m_layerStates.at(layers->size()).color);

    int masterIndex = -1;
    for (int i = 0; i < m_layerStates.size(); ++i) {
        const LayerState &state = m_layerStates.at(i);
        Layer *layer = layers->at(i);
        layer->setName(state.name);
        layer->setColor(state.color);
        layer->setVisible(state.visible);
        layer->setLocked(state.locked);
        if (state.master)
            masterIndex = i;
    }
    if (masterIndex >= 0)
        layerList.setMaster(masterIndex);

    // One rebuild + one appearance resync for the whole batch: the combo
    // boxes/list views repopulate and every connected Sheet refreshes its
    // primitives' visibility/selectability against the restored states.
    layerList.update();
    layerList.notifyAppearanceChanged();
}
