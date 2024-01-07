#include "LayerListView.h"

LayerListView::LayerListView(QWidget *parent) : QListWidget(parent)
{
    addLayerList(LayerList::getInstance().getList());
}

void LayerListView::addLayer(Layer *layer)
{
    QListWidgetItem *item = new QListWidgetItem(this);
    item->setData(Qt::UserRole+5, QVariant::fromValue(layer));
    item->setSizeHint(QSize(100, 40));

    QWidget *widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(widget);

    layout->setContentsMargins(2,0,4,0);

    // Clickable Icon
    ButtonLayerHide *lickableIconLabel = new ButtonLayerHide(layer, widget);

    layout->addWidget(lickableIconLabel);


    // Color Picker
    ColorPicker *colorPicker = new ColorPicker(widget);
    colorPicker->setColor(layer->color());
    colorPicker->setFixedSize(25,25);
    layout->addWidget(colorPicker);

    // Text
    LabelLayerName *label = new LabelLayerName(layer->name(),widget);
    layout->addWidget(label);

    if(layer->isMaster())
    {
        QLabel *nonClickableIconLabel = new QLabel(widget);
        QIcon icon = QIcon(":/res/resources/remix/bookmark-3-line.png");
        QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(28, 28)));
        nonClickableIconLabel->setPixmap(pixmap);
        nonClickableIconLabel->setFixedSize(28,28);
        layout->addWidget(nonClickableIconLabel);
    }
    widget->setLayout(layout);
    setItemWidget(item, widget);
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
