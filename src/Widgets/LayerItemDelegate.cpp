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
#include "LayerIcons.h"
#include "Layer.h"
#include "ThemeManager.h"

LayerItemDelegate::LayerItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

QRect LayerItemDelegate::eyeIconRect(const QRect &itemRect)
{
    return QRect(itemRect.x() + 2, itemRect.y() + (itemRect.height() - 20) / 2, 20, 20);
}

QRect LayerItemDelegate::lockIconRect(const QRect &itemRect)
{
    return QRect(itemRect.x() + 24, itemRect.y() + (itemRect.height() - 20) / 2, 20, 20);
}

QRect LayerItemDelegate::colorSwatchRect(const QRect &itemRect)
{
    return QRect(itemRect.x() + 46, itemRect.y() + (itemRect.height() - 20) / 2, 20, 20);
}

QRect LayerItemDelegate::textRect(const QRect &itemRect)
{
    return itemRect.adjusted(71, 0, 0, 0);
}

void LayerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QRect rect = opt.rect;

    // Draw the item's background rectangle using the hover background color
    if (opt.state & QStyle::State_MouseOver) {
        opt.palette.setColor(QPalette::Highlight, QColor(200, 200, 200));
        painter->fillRect(opt.rect, opt.palette.highlight());
    }

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // Eye/lock toggle icons - clickable from LayerComboBox's own popup (see
    // its eventFilter()). Always read the layer's *live* state (no cached
    // bool in the model), so a toggle from elsewhere (DialogLayerList) is
    // reflected the next time this row repaints, with no sync bookkeeping.
    Layer *layer = index.data(Qt::UserRole + 2).value<Layer *>();
    if (layer) {
        // themedIcon(): the glyphs are black line art, invisible on the
        // dark themes' surfaces without the light re-tint.
        const QPixmap eyeIcon(layer->isVisible()
                ? QStringLiteral(":/res/resources/remix/eye-line.png")
                : QStringLiteral(":/res/resources/remix/eye-off-line.png"));
        painter->drawPixmap(eyeIconRect(rect), ThemeManager::themedIcon(eyeIcon));
        painter->drawPixmap(lockIconRect(rect), ThemeManager::themedIcon(
                                LayerIcons::renderLockIcon(layer->isLocked())));
    }

    // Draw the color swatch
    QColor colore = index.data(Qt::UserRole + 1).value<QColor>();
    painter->fillRect(colorSwatchRect(rect), colore);

    // Set the text color
    painter->setPen(opt.palette.color(QPalette::WindowText));

    // Draw the text next to the swatch
    QString testo = index.data(Qt::DisplayRole).toString();
    painter->drawText(textRect(rect), Qt::AlignVCenter, testo);
}

QSize LayerItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    // Add a margin to increase the spacing between combobox rows
    QSize hint = QStyledItemDelegate::sizeHint(option, index);
    hint.setHeight(hint.height() + 2);
    hint.setWidth(hint.width() - 40 + 46); // existing fudge, plus the eye+lock icon boxes
    return hint;
}
