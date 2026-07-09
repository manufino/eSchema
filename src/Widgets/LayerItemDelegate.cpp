/*
 * LayerItemDelegate.cpp
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

#include "LayerItemDelegate.h"

LayerItemDelegate::LayerItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void LayerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QRect rect = opt.rect;

    // Draw the item's background rectangle using the hover background color
    if (opt.state & QStyle::State_MouseOver) {
        opt.palette.setColor(QPalette::Highlight, QColor(200, 200, 200));
        painter->fillRect(opt.rect, opt.palette.highlight());
    }

    // Draw the color swatch
    QColor colore = index.data(Qt::UserRole + 1).value<QColor>();
    QRect colorRect = QRect(rect.x(), rect.y(), 20, 20);
    painter->fillRect(colorRect, colore);

    // Set the text color
    painter->setPen(opt.palette.color(QPalette::WindowText));

    // Draw the text next to the swatch
    QString testo = index.data(Qt::DisplayRole).toString();
    QRect textRect = opt.rect.adjusted(25, 0, 0, 0); // Move the text to the right of the swatch
    painter->drawText(textRect, Qt::AlignVCenter, testo);
}

QSize LayerItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    // Add a margin to increase the spacing between combobox rows
    QSize hint = QStyledItemDelegate::sizeHint(option, index);
    hint.setHeight(hint.height() + 2);
    hint.setWidth(hint.width() - 40);
    return hint;
}
