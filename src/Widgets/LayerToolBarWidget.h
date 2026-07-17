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

// The main toolbar's layer chooser: a thin .ui wrapper around LayerComboBox
// that keeps the combo synced with the global LayerList and makes the
// selected layer the master (the one new primitives are drawn on).
class LayerToolBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LayerToolBarWidget(QWidget *parent = nullptr);
    ~LayerToolBarWidget();

public slots:
    // Moves the combo's selection onto `layer` without changing any state -
    // used when the master changes from elsewhere (e.g. the layer manager).
    void setMaster(Layer *layer);
    // Repopulates the combo from `layerList` - connected to
    // LayerList::layerListChanged.
    void setList(QList<Layer*> *layerList);

private slots:
    // The user picked a row: flags that layer as master via
    // LayerList::setMaster().
    void selectedChanged(int index);

private:
    Ui::LayerToolBarWidget *ui;
};

#endif // LAYERTOOLBARWIDGET_H
