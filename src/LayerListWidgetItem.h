#ifndef LAYERLISTWIDGETITEM_H
#define LAYERLISTWIDGETITEM_H

#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QLabel>

#include "Layer.h"
#include "LayerVisibilityButton.h"
#include "LayerLabelName.h"
#include "LayerColorPicker.h"


class LayerListWidgetItem : public QListWidgetItem {
public:
    LayerListWidgetItem(Layer *layer, QListWidget *parent = nullptr);
    QWidget *getWidget() { return widget; }

private:
    QWidget *widget;
};


#endif // LAYERLISTWIDGETITEM_H
