/*
 * ColorPicker.h
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

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QLabel>
#include <QColorDialog>
#include <QMouseEvent>

class ColorPicker : public QLabel
{
    Q_OBJECT

public:
    ColorPicker(QWidget *parent = nullptr);

    QColor getColor() { return m_color; }
    void setColor(QColor color);

signals:
    void colorChanged(QColor color);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QColor m_color;

};

#endif // COLORPICKER_H
