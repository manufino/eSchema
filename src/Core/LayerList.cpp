/*
 * LayerList.cpp
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

#include "LayerList.h"

void LayerList::addLayer(Layer *layer)
{
    layerList->append(layer);
    emit layerListChanged(layerList);
}

void LayerList::addLayer(QString name, QColor color)
{
    Layer *layer = new Layer(name, color);
    layerList->append(layer);
    emit layerListChanged(layerList);
}

Layer* LayerList::getMaster()
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer *currentLayer = const_cast<Layer*>(layerList->at(i));
        if(currentLayer->isMaster())
            return currentLayer;
    }
    return nullptr;
}

void LayerList::moveUp(Layer *layer)
{
    int currentIndex = layerList->indexOf(layer);

    if (currentIndex > 0) {
        layerList->move(currentIndex, currentIndex - 1);
        emit layerListChanged(layerList);
    }
}

void LayerList::moveDown(Layer *layer)
{
    int currentIndex = layerList->indexOf(layer);

    if (currentIndex < layerList->count() - 1) {
        layerList->move(currentIndex, currentIndex + 1);
        emit layerListChanged(layerList);
    }
}

void LayerList::update()
{
    emit layerListChanged(layerList);
}

void LayerList::removeLayer(Layer *layer)
{
    int index = layerList->indexOf(layer);

    if (index != -1) {
        layerList->removeAt(index);
        delete layer; // free the memory
        emit layerListChanged(layerList);
    }
}

void LayerList::setMaster(Layer* layer)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* currentLayer = (*layerList)[i];
        // the master layer must always be visible
        if(currentLayer->isMaster())
            currentLayer->setVisible(true);
        currentLayer->setMaster(currentLayer == layer);
    }
}

void LayerList::setMaster(QString name)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* l = (*layerList)[i];

        if(l->name() == name) {
            l->setMaster(true);
            l->setVisible(true);// the master layer must always be visible
        } else l->setMaster(false);
    }
}

void LayerList::setMaster(int index)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* l = (*layerList)[i];

        l->setMaster(i == index);

        // the master layer must always be visible
        if(l->isMaster())
            l->setVisible(true);
    }
}

void LayerList::setVisible(Layer* layer, bool visible)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* currentLayer = (*layerList)[i];
        if (currentLayer == layer)
        {
            // don't allow hiding the master layer
            if(visible == false && layer->isMaster())
                return;

            currentLayer->setVisible(visible);
            //emit layerListChanged(layerList);
            break;
        }
    }
}

void LayerList::setAllVisibleOrHidden(bool visible)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* currentLayer = (*layerList)[i];

        // don't allow hiding the master layer
        if(currentLayer->isMaster())
            continue;

        currentLayer->setVisible(visible);
    }
}
