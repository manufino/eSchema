/*
 * LayerColorPicker.h
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

#ifndef LAYERCOLORPICKER_H
#define LAYERCOLORPICKER_H

#include <QObject>
#include <QWidget>

#include "ColorPicker.h"
#include "Layer.h"
#include "LayerList.h"

class LayerColorPicker : public ColorPicker
{
    Q_OBJECT
public:
    LayerColorPicker(Layer *layer, QWidget *parent = nullptr);

private slots:
    void layerColorChanged(QColor color);

private:
    Layer *layer;
};

#endif // LAYERCOLORPICKER_H
