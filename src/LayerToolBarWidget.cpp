#include "LayerToolBarWidget.h"
#include "ui_LayerToolBarWidget.h"

LayerToolBarWidget::LayerToolBarWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LayerToolBarWidget)
{
    ui->setupUi(this);

    connect(&LayerList::getInstance(),
    &LayerList::layerListChanged,
    this, &LayerToolBarWidget::setList);

    connect(ui->comboBox,
    &LayerComboBox::layerSelectedChanged,
    this, &LayerToolBarWidget::selectedChanged);
}

LayerToolBarWidget::~LayerToolBarWidget()
{
    delete ui;
}

void LayerToolBarWidget::setMaster(Layer *layer)
{
    ui->comboBox->setMaster(layer);
}

void LayerToolBarWidget::setList(QList<Layer*> *layerList)
{
    ui->comboBox->addLayerList(layerList);
}

void LayerToolBarWidget::selectedChanged(int index)
{
    if(index >= 0 && ui->comboBox->count() > 0)
        LayerList::getInstance().setMaster(ui->comboBox->currentData(Qt::DisplayRole).toString());
}
