/*
 * DialogFind.h
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

#ifndef DIALOGFIND_H
#define DIALOGFIND_H

#include <QDialog>
#include <QString>

namespace Ui {
class DialogFind;
}

// A small non-modal "find in drawing" panel (Ctrl+F) - unlike every other
// dialog in this app, it stays open while the user keeps working on the
// canvas. Doesn't search anything itself: only MainWindow has access to the
// sheet, so this just reports the current search text and asks for the
// next/previous match - see MainWindow::findInDrawing().
class DialogFind : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFind(QWidget *parent = nullptr);
    ~DialogFind();

    // Shown next to the buttons, e.g. "2 of 5" or "No matches".
    void setStatusText(const QString &text);

signals:
    void findNext(const QString &text);
    void findPrevious(const QString &text);

private:
    Ui::DialogFind *ui;
};

#endif // DIALOGFIND_H
