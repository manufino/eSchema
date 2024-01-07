#ifndef LAYERCOLORPICKER_H
#define LAYERCOLORPICKER_H

#include <QObject>
#include <QWidget>

#include "ColorPicker.h"
#include "Layer.h"
#include "LayerList.h"

class LayerColorPicker : public ColorPicker
{
    Q_OBJECT
public:
    LayerColorPicker(Layer *layer, QWidget *parent = nullptr);

private slots:
    void layerColorChanged(QColor color);

private:
    Layer *layer;
};

#endif // LAYERCOLORPICKER_H
