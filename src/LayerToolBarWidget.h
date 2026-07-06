/*
 * LayerToolBarWidget.h
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
    void setList(QList<Layer*> *layerList);

private slots:
    void selectedChanged(int index);

private:
    Ui::LayerToolBarWidget *ui;
};

#endif // LAYERTOOLBARWIDGET_H
