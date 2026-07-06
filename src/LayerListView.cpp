/*
 * LayerListView.cpp
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

#include "LayerListView.h"

LayerListView::LayerListView(QWidget *parent) : QListWidget(parent)
{
    addLayerList(LayerList::getInstance().getList());
}

void LayerListView::addLayer(Layer *layer)
{
    LayerListWidgetItem *item = new LayerListWidgetItem(layer, this);
    setItemWidget(item, item->getWidget());
}

void LayerListView::updateList()
{
    this->clear();
    addLayerList(LayerList::getInstance().getList());
}

void LayerListView::addLayerList(QList<Layer*> *layerList)
{
    this->clear();
    for (Layer *layer : *layerList)
        addLayer(layer);
}

Layer* LayerListView::getSelectedLayer()
{
    // se non ci sono item selezionati ritorna un nullptr
    if(selectedItems().size() == 0)
        return nullptr;

    QListWidgetItem *selectedItem = currentItem();

    if (selectedItem != nullptr && selectedItem->data(Qt::UserRole+5).isValid())
        return qvariant_cast<Layer*>(selectedItem->data(Qt::UserRole+5));
    else
        return nullptr;
}

void LayerListView::setSelectedLayer(Layer *layer)
{
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *item = this->item(i);
        if (item != nullptr && item->data(Qt::UserRole+5).isValid()) {
            Layer *currentLayer = qvariant_cast<Layer*>(item->data(Qt::UserRole+5));
            if (currentLayer == layer) {
                setCurrentItem(item); // Imposta l'elemento corrente sulla base del layer passato
                return;
            }
        }
    }
    // Se il layer passato non è stato trovato, deseleziona tutti gli elementi
    setCurrentItem(nullptr);
}
