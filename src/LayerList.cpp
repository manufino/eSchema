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
        delete layer; // libero la memoria
        emit layerListChanged(layerList);
    }
}

void LayerList::setMaster(Layer* layer)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* currentLayer = (*layerList)[i];
        // il layer master deve essere sempre visibile
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
            l->setVisible(true);// il layer master deve essere sempre visibile
        } else l->setMaster(false);
    }
}

void LayerList::setMaster(int index)
{
    for (int i = 0; i < layerList->count(); i++)
    {
        Layer* l = (*layerList)[i];

        l->setMaster(i == index);

        // il layer master deve essere sempre visibile
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
            // non permetto di nascondere il layer master
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

        // non permetto di nascondere il layer master
        if(currentLayer->isMaster())
            continue;

        currentLayer->setVisible(visible);
    }
}
