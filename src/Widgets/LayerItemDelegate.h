/*
 * LayerItemDelegate.h
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

#ifndef LAYERITEMDELEGATE_H
#define LAYERITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QRect>

// Paints one row of LayerComboBox's popup: visibility eye, lock, color
// swatch and layer name, laid out by the shared rect helpers below.
class LayerItemDelegate : public QStyledItemDelegate {
public:
    LayerItemDelegate(QObject* parent = nullptr);
    // Draws the eye/lock (per the row's Layer state), the color swatch and
    // the name; dims the row when the layer is hidden.
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    // Icon hit-rects within a row, shared with LayerComboBox's event filter
    // (see its eventFilter()) so drawing and click hit-testing can never
    // drift apart - both simply call these same two functions.
    static QRect eyeIconRect(const QRect &itemRect);
    static QRect lockIconRect(const QRect &itemRect);
    static QRect colorSwatchRect(const QRect &itemRect);
    static QRect textRect(const QRect &itemRect);
};

#endif // LAYERITEMDELEGATE_H
