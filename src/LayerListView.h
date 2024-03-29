#ifndef LAYERLISTVIEW_H
#define LAYERLISTVIEW_H


#include <QListWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QLineEdit>

#include "Layer.h"
#include "ColorPicker.h"
#include "LayerList.h"
#include "LayerVisibilityButton.h"
#include "LayerLabelName.h"
#include "LayerListWidgetItem.h"


class LayerListView : public QListWidget {
    Q_OBJECT
public:
    LayerListView(QWidget *parent = nullptr);
    void addLayer(Layer *layer);
    void updateList();
    Layer* getSelectedLayer();
    void setSelectedLayer(Layer *layer);

public slots:
    void addLayerList(QList<Layer*> *layerList);

private:

};


#endif // LAYERLISTVIEW_H
