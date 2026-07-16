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

void LayerList::createDefaultLayers()
{
    if (!layerList->isEmpty())
        return;

    static const QColor colors[16] = {
        QColor(0, 0, 0),        QColor(0, 0, 128),      QColor(255, 0, 0),      QColor(0, 128, 128),
        QColor(255, 200, 0),    QColor(127, 255, 0),    QColor(0, 255, 255),    QColor(0, 128, 0),
        QColor(154, 205, 50),   QColor(255, 20, 147),   QColor(181, 155, 12),   QColor(1, 128, 255),
        QColor(225, 225, 225, 242), QColor(162, 162, 162, 230), QColor(95, 95, 95, 230), QColor(0, 0, 0)
    };
    for (int i = 0; i < 16; ++i)
        addLayer(new Layer(QString("Layer %1").arg(i), colors[i], i == 0));
}

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
        // Let every raw-pointer holder (Sheet's primitives, combo models)
        // drop or reassign the layer while it's still alive - deleting
        // first left them all dangling (crash at the next repaint).
        emit layerAboutToBeRemoved(layer);
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
        const bool isNewMaster = currentLayer == layer;
        currentLayer->setMaster(isNewMaster);
        // The master layer must always be visible - the *new* master, not
        // whichever happened to be master before this loop flipped it.
        if (isNewMaster)
            currentLayer->setVisible(true);
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
            emit layerAppearanceChanged();
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
    emit layerAppearanceChanged();
}

void LayerList::setLocked(Layer* layer, bool locked)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* currentLayer = (*layerList)[i];
        if (currentLayer == layer)
        {
            // don't allow locking the master layer - avoids locking
            // yourself out of the layer you're actively drawing on
            if(locked && layer->isMaster())
                return;

            currentLayer->setLocked(locked);
            emit layerAppearanceChanged();
            break;
        }
    }
}

void LayerList::setAllLockedOrUnlocked(bool locked)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* currentLayer = (*layerList)[i];

        // don't allow locking the master layer
        if(locked && currentLayer->isMaster())
            continue;

        currentLayer->setLocked(locked);
    }
    emit layerAppearanceChanged();
}
