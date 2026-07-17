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


// The layer manager dialog's list (DialogLayerList): one composite row per
// layer in the global LayerList (visibility eye, lock, color swatch,
// editable name, master bookmark - see LayerListWidgetItem). Rebuilds all
// rows on every settings change so a live theme switch re-tints the glyphs.
class LayerListView : public QListWidget {
    Q_OBJECT
public:
    LayerListView(QWidget *parent = nullptr);
    // Appends one composite row for `layer`.
    void addLayer(Layer *layer);
    // Clears and rebuilds every row from the global LayerList - the
    // catch-all refresh DialogLayerList calls after any layer change.
    void updateList();
    // The Layer of the currently selected row, or nullptr with no selection.
    Layer* getSelectedLayer();
    // Selects the row showing `layer` (clearing the selection when the
    // layer isn't listed) - used to keep the selection across rebuilds.
    void setSelectedLayer(Layer *layer);

public slots:
    // Clears and repopulates from an explicit roster - also the
    // constructor's initial fill.
    void addLayerList(QList<Layer*> *layerList);

private:

};


#endif // LAYERLISTVIEW_H
