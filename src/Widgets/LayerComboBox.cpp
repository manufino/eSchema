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
#include "LayerIcons.h"
#include "SettingsManager.h"
#include "ThemeManager.h"
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

    // Every instance keeps itself in sync with the singleton list: an
    // add/remove/reorder/rename rebuilds the rows. Without this, only the
    // toolbar instance was ever refreshed - the properties-panel one kept
    // stale rows forever, holding dangling Layer* item data after a
    // deletion and silently mapping its positional selection onto the
    // wrong live layer after a reorder.
    connect(&LayerList::getInstance(), &LayerList::layerListChanged,
            this, [this](QList<Layer *> *list) { addLayerList(list); });

    // A live theme switch saves "gui_style": rebuilding the rows re-tints
    // the item icons (built with ThemeManager::themedIcon() at row-build
    // time) for the new theme without reopening anything.
    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, [this]() { addLayerList(LayerList::getInstance().getList()); });

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

    // Draw the eye/lock state, color swatch and name of the current layer -
    // display-only here (not clickable: the current layer is always the
    // master one, for which both are permanently forced to visible/
    // unlocked, see LayerList::setVisible()/setLocked()'s master guards),
    // so at a glance the closed combobox already shows what the popup would.
    if (currentIndex() >= 0 && currentIndex() < count()) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        if (Layer *layer = selectedLayer()) {
            const QRect rowRect(0, 0, opt.rect.height(), opt.rect.height());
            // themedIcon(): black line art, invisible on the dark themes'
            // surfaces without the light re-tint.
            const QPixmap eyeIcon(layer->isVisible()
                    ? QStringLiteral(":/res/resources/remix/eye-line.png")
                    : QStringLiteral(":/res/resources/remix/eye-off-line.png"));
            painter.drawPixmap(LayerItemDelegate::eyeIconRect(rowRect),
                                ThemeManager::themedIcon(eyeIcon));
            painter.drawPixmap(LayerItemDelegate::lockIconRect(rowRect),
                                ThemeManager::themedIcon(
                                        LayerIcons::renderLockIcon(layer->isLocked())));
        }

        QRect colorRect = QRect(46, 5, 20, 14);
        QColor colore = itemData(currentIndex(), Qt::UserRole + 1).value<QColor>();
        painter.fillRect(colorRect, colore);

        QRect textRect = opt.rect.adjusted(71, 0, 0, 0);
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
    hint.setWidth(maxWidth + 112); // + eye/lock icon boxes drawn in paintEvent()
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
    // QComboBox's popup selects-and-closes on *release* (its "activated"
    // click), not on press - swallowing only MouseButtonPress left the
    // paired release free to fall through and close the popup anyway.
    // Every event of the gesture (press/release/double-click) landing on an
    // icon must be swallowed, and the actual toggle only applied once (on
    // press), or a same-position double-click would toggle twice.
    const bool isMouseGesture = event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::MouseButtonRelease
            || event->type() == QEvent::MouseButtonDblClick;
    if (view() && watched == view()->viewport() && isMouseGesture) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QModelIndex index = view()->indexAt(mouseEvent->pos());
        if (index.isValid()) {
            Layer *layer = index.data(Qt::UserRole + 2).value<Layer *>();
            const QRect itemRect = view()->visualRect(index);
            const bool onEye = layer && LayerItemDelegate::eyeIconRect(itemRect).contains(mouseEvent->pos());
            const bool onLock = layer && LayerItemDelegate::lockIconRect(itemRect).contains(mouseEvent->pos());
            if (onEye || onLock) {
                if (event->type() == QEvent::MouseButtonPress) {
                    if (onEye)
                        LayerList::getInstance().setVisible(layer, !layer->isVisible());
                    else
                        LayerList::getInstance().setLocked(layer, !layer->isLocked());
                    view()->viewport()->update();
                }
                return true; // swallow - keep the popup open, don't select this row
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
