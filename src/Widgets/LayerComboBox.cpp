/*
 * LayerComboBox.cpp
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

#include "LayerComboBox.h"
#include "LayerItemDelegate.h"
#include "qpainterpath.h"
#include <QAbstractItemView>
#include <QMouseEvent>

LayerComboBox::LayerComboBox(QWidget* parent)
    : QComboBox(parent)
{
    setItemDelegate(new LayerItemDelegate(this));
    QList<Layer*> *la = LayerList::getInstance().getList();
    addLayerList(la);
    connect(this, &QComboBox::currentIndexChanged,
    this, &LayerComboBox::currentIndexChanged);

    // view() lazily creates the popup's QAbstractItemView - forcing that
    // here lets the eye/lock click interception (eventFilter()) be wired up
    // once, up front.
    view()->viewport()->installEventFilter(this);
}

void LayerComboBox::addLayer(Layer *layer)
{
    QStandardItem* item = new QStandardItem;
    item->setData(QColor(layer->color()), Qt::UserRole + 1);
    item->setData(layer->name(), Qt::DisplayRole);
    // The live Layer* itself (not a cached visible/locked snapshot) - see
    // LayerItemDelegate::paint(), which always reads current state from it.
    item->setData(QVariant::fromValue(layer), Qt::UserRole + 2);

    QStandardItemModel* standardModel = qobject_cast<QStandardItemModel*>(model());
    if (standardModel)
        standardModel->insertRow(standardModel->rowCount(), item);
}

void LayerComboBox::addLayerList(QList<Layer*> *list)
{
    // Disable the signal while loading the list
    disconnect(this, &QComboBox::currentIndexChanged,
               this, &LayerComboBox::currentIndexChanged);

    clear();
    layerList = list;
    for(Layer *layer: *list)
        addLayer(layer);

    setAutoMaster(); // set the master layer

    // Re-enable the signal
    connect(this, &QComboBox::currentIndexChanged,
            this, &LayerComboBox::currentIndexChanged);
}

void LayerComboBox::setMaster(Layer *layer)
{
    int index = -1;
    for (int i = 0; i < layerList->size(); ++i) {
        if (layerList->at(i) == layer) {
            index = i;
            break;
        }
    }
    setCurrentIndex(index);
}

void LayerComboBox::setAutoMaster()
{
    for (int i = 0; i < layerList->size(); ++i) {
        if (layerList->at(i)->isMaster()) {
            setCurrentIndex(i);
            return;
        }
    }
}

void LayerComboBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // Only draw the combobox's own chrome (border, arrow, etc.)
    style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);

    // Draw the color swatch
    if (currentIndex() >= 0 && currentIndex() < count()) {
        painter.setRenderHint(QPainter::Antialiasing);
        QRect colorRect = QRect(5, 5, 20, 14);
        QColor colore = itemData(currentIndex(), Qt::UserRole + 1).value<QColor>();
        painter.fillRect(colorRect, colore);
        /*
        QPainterPath path;
        path.addRoundedRect(colorRect, 4, 4);
        QPen pen(colore, 10);
        painter.setPen(pen);
        painter.fillPath(path, colore);
        painter.drawPath(path);
        */
        QRect textRect = opt.rect.adjusted(28, 0, 0, 0);
        painter.setPen(opt.palette.color(QPalette::WindowText));
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, currentText());
    }
}

QSize LayerComboBox::sizeHint() const
{
    QSize hint = QComboBox::sizeHint();
    // TODO: needs verification.
    // Find the widest text among the individual items
    int maxWidth = 0;
    for (int i = 0; i < count(); ++i) {
        QString text = itemText(i);
        int textWidth = fontMetrics().horizontalAdvance(text);
        maxWidth = qMax(maxWidth, textWidth);
    }
    hint.setWidth(maxWidth + 70);
    return hint;
}

Layer *LayerComboBox::selectedLayer() const
{
    if (!layerList || currentIndex() < 0 || currentIndex() >= layerList->size())
        return nullptr;
    return layerList->at(currentIndex());
}

bool LayerComboBox::eventFilter(QObject *watched, QEvent *event)
{
    if (view() && watched == view()->viewport() && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QModelIndex index = view()->indexAt(mouseEvent->pos());
        if (index.isValid()) {
            Layer *layer = index.data(Qt::UserRole + 2).value<Layer *>();
            const QRect itemRect = view()->visualRect(index);
            if (layer && LayerItemDelegate::eyeIconRect(itemRect).contains(mouseEvent->pos())) {
                LayerList::getInstance().setVisible(layer, !layer->isVisible());
                view()->viewport()->update();
                return true; // swallow - keep the popup open, don't select this row
            }
            if (layer && LayerItemDelegate::lockIconRect(itemRect).contains(mouseEvent->pos())) {
                LayerList::getInstance().setLocked(layer, !layer->isLocked());
                view()->viewport()->update();
                return true;
            }
        }
    }
    return QComboBox::eventFilter(watched, event);
}

void LayerComboBox::currentIndexChanged(int index)
{
    Q_UNUSED(index);

    emit layerSelectedChanged(currentIndex());
}

void LayerComboBox::layerListIsChanged(QList<Layer*> *layerList)
{
    this->layerList = layerList;
}

bool LayerComboBox::layerNameIsUnique(QString name)
{
    for(Layer *layer : *layerList)
        if(layer->name() == name)
            return false;

    return true;
}
