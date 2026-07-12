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


class LayerComboBox : public QComboBox {
    Q_OBJECT
public:
    LayerComboBox(QWidget* parent = nullptr);
    void addLayer(Layer *layer);
    void addLayerList(QList<Layer*> *list);
    void setMaster(Layer *layer);
    void setAutoMaster();
    Layer *selectedLayer() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    // Intercepts clicks on the popup's eye/lock icons (see
    // LayerItemDelegate::eyeIconRect()/lockIconRect()) so they toggle
    // visibility/lock in place instead of being treated as "select this row
    // and close the popup" - installed on view()->viewport() in the
    // constructor.
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void layerSelectedChanged(int i);

public slots:
    void layerListIsChanged(QList<Layer*> *layerList);

private slots:
    void currentIndexChanged(int index);

private:
    bool layerNameIsUnique(QString name);


private:
    QList<Layer*> *layerList;
};

#endif // LAYERCOMBOBOX_H
