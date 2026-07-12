/*
 * LayerLockButton.cpp
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

#include "LayerLockButton.h"
#include "LayerIcons.h"

LayerLockButton::LayerLockButton(Layer *layer, QWidget *parent)
    : QLabel(parent)
{
    this->layer = layer;

    images.append(LayerIcons::renderLockIcon(false));
    images.append(LayerIcons::renderLockIcon(true));

    setStatus(layer->isLocked());

    setMouseTracking(true);
    setCursor(layer->isMaster() ? Qt::ArrowCursor : Qt::PointingHandCursor);
    setText("");
}

void LayerLockButton::setStatus(bool status)
{
    setPixmap(images[status ? 1 : 0]);

    layerIsLocked = status;
    LayerList::getInstance().setLocked(layer, layerIsLocked);
}

void LayerLockButton::mousePressEvent(QMouseEvent *event)
{
    if (layer->isMaster())
        return;

    if (event->button() == Qt::LeftButton)
        setStatus(!layerIsLocked);

    QLabel::mousePressEvent(event);
}

void LayerLockButton::mouseMoveEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    if (layer->isMaster())
        setCursor(Qt::ArrowCursor);
    else
        setCursor(Qt::PointingHandCursor);
}
