/*
 * LayerListView.h
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
