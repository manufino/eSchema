#include "LayerToolBarWidget.h"
#include "ui_LayerToolBarWidget.h"

LayerToolBarWidget::LayerToolBarWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LayerToolBarWidget)
{
    ui->setupUi(this);
}

LayerToolBarWidget::~LayerToolBarWidget()
{
    delete ui;
}
