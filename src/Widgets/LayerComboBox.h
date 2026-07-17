/*
 * LayerComboBox.h
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

#ifndef LAYERCOMBOBOX_H
#define LAYERCOMBOBOX_H

#include <QComboBox>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyleOptionComboBox>

#include "LayerItemDelegate.h"
#include "Layer.h"
#include "LayerList.h"


// The layer combo box (toolbar + properties panel): each row shows the
// layer's eye/lock/color/name via LayerItemDelegate, and the eye/lock icons
// are directly clickable inside the popup. Kept in sync with the global
// LayerList through layerListIsChanged().
class LayerComboBox : public QComboBox {
    Q_OBJECT
public:
    LayerComboBox(QWidget* parent = nullptr);
    // Appends one row for `layer` (the Layer* is stored as item data).
    void addLayer(Layer *layer);
    // Clears and repopulates from `list`, keeping the selection if possible.
    void addLayerList(QList<Layer*> *list);
    // Moves the selection onto `layer` without side effects.
    void setMaster(Layer *layer);
    // Selects whatever layer the global LayerList currently flags as master.
    void setAutoMaster();
    // The Layer of the current row, or nullptr for an empty combo.
    Layer *selectedLayer() const;

protected:
    // Draws the closed combo like a popup row (color swatch + name) instead
    // of Qt's default text-only rendering.
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    // Intercepts clicks on the popup's eye/lock icons (see
    // LayerItemDelegate::eyeIconRect()/lockIconRect()) so they toggle
    // visibility/lock in place instead of being treated as "select this row
    // and close the popup" - installed on view()->viewport() in the
    // constructor.
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    // The user picked a different row (index into the roster) - the layer
    // toolbar widget turns this into LayerList::setMaster().
    void layerSelectedChanged(int i);

public slots:
    // Full rebuild from the given roster - connected to
    // LayerList::layerListChanged.
    void layerListIsChanged(QList<Layer*> *layerList);

private slots:
    // Re-emits a user row change as layerSelectedChanged().
    void currentIndexChanged(int index);

private:
    // True when no existing layer already uses `name` - guards the
    // "add new layer" flow.
    bool layerNameIsUnique(QString name);


private:
    QList<Layer*> *layerList;
};

#endif // LAYERCOMBOBOX_H
