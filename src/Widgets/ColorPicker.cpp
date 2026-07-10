/*
 * ColorPicker.cpp
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

#include "ColorPicker.h"

ColorPicker::ColorPicker(QWidget *parent) : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setAutoFillBackground(true);
    m_color = QColor("black");
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Cambia colore ..."));
    setProperty("class", "labelColor");
}

void ColorPicker::setColor(QColor color)
{
    m_color = color;
    setStyleSheet(QString(".labelColor {background-color: %1;}").arg(m_color.name()));
}

void ColorPicker::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QColorDialog colorDialog(this);
        QColor color = colorDialog.getColor(getColor(), this, tr("Seleziona colore"));

        if (color.isValid()) {
            setColor(color);
            emit colorChanged(color);
        }
    }
}
