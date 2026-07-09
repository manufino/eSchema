/*
 * LayerListWidgetItem.cpp
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

#include "LayerListWidgetItem.h"

LayerListWidgetItem::LayerListWidgetItem(Layer *layer,  QListWidget *parent) : QListWidgetItem(parent) {
    setData(Qt::UserRole+5, QVariant::fromValue(layer));
    setSizeHint(QSize(100, 40));

    widget = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(2, 0, 4, 0);

    // show or hide the layer
    LayerVisibilityButton *lickableIconLabel = new LayerVisibilityButton(layer, widget);
    layout->addWidget(lickableIconLabel);

    // layer color
    LayerColorPicker *colorPicker = new LayerColorPicker(layer, widget);
    layout->addWidget(colorPicker);

    // layer name
    LayerLabelName *label = new LayerLabelName(layer, widget);
    layout->addWidget(label);

    // only add the icon on the master layer
    if (layer->isMaster()) {
        QLabel *nonClickableIconLabel = new QLabel(widget);
        QIcon icon = QIcon(":/res/resources/remix/bookmark-3-line.png");
        QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(28, 28)));
        nonClickableIconLabel->setPixmap(pixmap);
        nonClickableIconLabel->setFixedSize(28, 28);
        layout->addWidget(nonClickableIconLabel);
    }
    widget->setLayout(layout);
}
