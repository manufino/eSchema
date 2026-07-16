/*
 * LayerVisibilityButton.cpp
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

#include "LayerVisibilityButton.h"
#include "ThemeManager.h"

LayerVisibilityButton::LayerVisibilityButton(Layer * layer, QWidget *parent)
    : QLabel(parent)
{
    this->layer = layer;

    // themedIcon(): the eye glyphs are black line art, invisible on the
    // dark themes' surfaces without the light re-tint. Applied once here -
    // these buttons live inside DialogLayerList, which is recreated on
    // every open, so a theme switch is picked up the next time it opens.
    images.append(ThemeManager::themedIcon(QPixmap(":/res/resources/remix/eye-line.png")));
    images.append(ThemeManager::themedIcon(QPixmap(":/res/resources/remix/eye-off-line.png")));
    images.append(ThemeManager::themedIcon(QPixmap(":/res/resources/remix/eye-fill.png")));

    if(layer->isMaster())
        setPixmap(images[2]);
    else
        setStatus(layer->isVisible());

    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setText("");
}

void LayerVisibilityButton::setStatus(bool status)
{
    if(status)
        setPixmap(images[0]);
    else
        setPixmap(images[1]);

    layerIsVisible = status;
    LayerList::getInstance().setVisible(layer, layerIsVisible);
}

void LayerVisibilityButton::mousePressEvent(QMouseEvent *event)
{
    if(layer->isMaster())
        return;

    if (event->button() == Qt::LeftButton)
        setStatus(!layerIsVisible);

    QLabel::mousePressEvent(event);
}

void LayerVisibilityButton::mouseMoveEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    if(layer->isMaster())
        setCursor(Qt::ArrowCursor);
    else setCursor(Qt::PointingHandCursor);
}
