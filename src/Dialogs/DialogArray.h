/*
 * DialogArray.h
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

#ifndef DIALOGARRAY_H
#define DIALOGARRAY_H

#include <QDialog>

namespace Ui { class DialogArray; }

// Parameters for Edit > Array of copies: replicate the selection on a
// columns x rows grid with the given steps (drawing units) between copies.
class DialogArray : public QDialog
{
    Q_OBJECT

public:
    explicit DialogArray(QWidget *parent = nullptr);
    ~DialogArray();

    int columns() const;
    int rows() const;
    qreal spacingX() const;
    qreal spacingY() const;

private:
    Ui::DialogArray *ui;
};

#endif // DIALOGARRAY_H
