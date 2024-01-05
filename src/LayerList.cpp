#include "LayerList.h"

void LayerList::addLayer(Layer layer) {
    layerList->append(layer);
    emit layerListChanged(layerList);
}

void LayerList::addLayer(QString name, QColor color) {
    Layer layer(name, color);
    layerList->append(layer);
    emit layerListChanged(layerList);
}

Layer* LayerList::getMaster() {
    for (int i = 0; i < layerList->count(); i++) {
        Layer &currentLayer = const_cast<Layer&>(layerList->at(i));
        if(currentLayer.isMaster())
            return &currentLayer;
    }
    return nullptr;
}

void LayerList::setMaster(Layer* layer) {
    for (int i = 0; i < layerList->count(); i++) {
        Layer& currentLayer = (*layerList)[i];
        currentLayer.setMaster(&currentLayer == layer);
    }
}

void LayerList::setMaster(int index) {
    for (int i = 0; i < layerList->count(); i++) {
        Layer& l = (*layerList)[i];
        l.setMaster(i == index);
    }
}
