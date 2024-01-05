#ifndef LAYERTOOLBARWIDGET_H
#define LAYERTOOLBARWIDGET_H

#include <QWidget>
#include "Layer.h"
#include "LayerComboBox.h"
#include "LayerList.h"

namespace Ui {
class LayerToolBarWidget;
}

class LayerToolBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LayerToolBarWidget(QWidget *parent = nullptr);
    ~LayerToolBarWidget();

public slots:
    void setMaster(Layer *layer);
    void setList(QList<Layer> *layerList);

private slots:
    void selectedChanged(int index);

private:
    Ui::LayerToolBarWidget *ui;
};

#endif // LAYERTOOLBARWIDGET_H
