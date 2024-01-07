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
    for (Layer *layer : *layerList) {
        addLayer(layer);
    }
}

Layer* LayerListView::getSelectedLayer()
{
    // se non ci sono item selezionati ritorna un nullptr
    if(selectedItems().size() == 0)
        return nullptr;

    QListWidgetItem *selectedItem = currentItem();

    if (selectedItem != nullptr && selectedItem->data(Qt::UserRole+5).isValid()) {
        return qvariant_cast<Layer*>(selectedItem->data(Qt::UserRole+5));
    } else {
        return nullptr; // Nessun layer selezionato o dati non validi
    }
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
