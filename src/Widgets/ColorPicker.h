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

// A clickable color swatch: a QLabel filled with the current color that
// opens a QColorDialog on left click and announces the chosen color via
// colorChanged(). Used directly in the Options dialog's color rows (as a
// promoted widget in the .ui files) and subclassed by LayerColorPicker.
class ColorPicker : public QLabel
{
    Q_OBJECT

public:
    ColorPicker(QWidget *parent = nullptr);

    // The currently displayed color.
    QColor getColor() { return m_color; }
    // Updates the swatch (background fill) - does NOT emit colorChanged(),
    // so callers can initialize it without triggering their own handler.
    void setColor(QColor color);

signals:
    // A new color was picked in the dialog (never fired on a cancel).
    void colorChanged(QColor color);

protected:
    // Left click opens the QColorDialog preloaded with the current color.
    void mousePressEvent(QMouseEvent *event) override;

private:
    QColor m_color;

};

#endif // COLORPICKER_H
