/*
 * LayerVisibilityButton.h
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

#ifndef LAYERVISIBILITYBUTTON_H
#define LAYERVISIBILITYBUTTON_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>

#include "Layer.h"
#include "LayerList.h"

class LayerVisibilityButton : public QLabel
{
    Q_OBJECT

public:
    explicit LayerVisibilityButton(Layer * layer, QWidget *parent = nullptr);
    void setStatus(bool status);
    bool getStatus() {return layerIsVisible;}

signals:
    void statusChanged(bool isVisible);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private slots:
    // Rebuilds the theme-tinted pixmaps and re-applies the current one -
    // connected to SettingsManager::settingIsChanged, so instances living
    // in the always-alive layer toolbar pick a live theme switch up too
    // (once-at-construction tinting only covered the recreated dialog).
    void refreshIcons();

private:
    QList<QPixmap> images;
    bool layerIsVisible;
    Layer *layer;
};

#endif // LAYERVISIBILITYBUTTON_H
