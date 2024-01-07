#include "LayerColorPicker.h"

LayerColorPicker::LayerColorPicker(Layer *layer, QWidget *parent) : ColorPicker(parent)
{
    this->layer = layer;

    setColor(layer->color());
    setFixedSize(25, 25);

    connect(this, &LayerColorPicker::colorChanged, this, &LayerColorPicker::layerColorChanged);
}

void LayerColorPicker::layerColorChanged(QColor color)
{
    layer->setColor(color);
    LayerList::getInstance().update();
}
