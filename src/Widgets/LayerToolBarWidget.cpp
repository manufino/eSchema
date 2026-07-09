/*
 * LayerToolBarWidget.cpp
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
