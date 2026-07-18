/*
 * DialogHatch.h
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

#ifndef DIALOGHATCH_H
#define DIALOGHATCH_H

#include <QDialog>

namespace Ui { class DialogHatch; }

// Parameters for Edit > Shape > Hatch: the angle and spacing of the fill
// lines, and whether a second, perpendicular family is added (cross-hatch).
// Only collects the values (persisted across sessions) - the actual line
// generation and clipping happens in MainWindow::clickHatchAction(), which
// turns them into plain LI primitives so the file format is untouched.
class DialogHatch : public QDialog
{
    Q_OBJECT

public:
    explicit DialogHatch(QWidget *parent = nullptr);
    ~DialogHatch();

    // Hatch-line direction in degrees, counterclockwise from horizontal.
    double angleDeg() const;
    // Distance between consecutive lines, in drawing units.
    double spacing() const;
    // True adds a second family rotated 90 degrees from the first.
    bool crossHatch() const;

    // QDialog::accept() plus persisting the chosen values for next time.
    void accept() override;

private:
    Ui::DialogHatch *ui;
};

#endif // DIALOGHATCH_H
