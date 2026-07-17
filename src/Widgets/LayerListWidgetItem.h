/*
 * LayerListWidgetItem.h
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

#ifndef LAYERLISTWIDGETITEM_H
#define LAYERLISTWIDGETITEM_H

#include <QListWidgetItem>
#include <QHBoxLayout>
#include <QLabel>

#include "Layer.h"
#include "LayerVisibilityButton.h"
#include "LayerLockButton.h"
#include "LayerLabelName.h"
#include "LayerColorPicker.h"


// One row of the layer manager's LayerListView: builds the composite row
// widget (eye, lock, color swatch, editable name, master bookmark) for a
// Layer and stores the Layer* in the item's data so
// LayerListView::getSelectedLayer() can recover it.
class LayerListWidgetItem : public QListWidgetItem {
public:
    LayerListWidgetItem(Layer *layer, QListWidget *parent = nullptr);
    // The composite row widget, to hand to QListWidget::setItemWidget().
    QWidget *getWidget() { return widget; }

private:
    QWidget *widget;
};


#endif // LAYERLISTWIDGETITEM_H
