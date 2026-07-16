/*
 * DialogPrintOptions.h
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

#ifndef DIALOGPRINTOPTIONS_H
#define DIALOGPRINTOPTIONS_H

#include <QDialog>

class Layer;

namespace Ui {
class DialogPrintOptions;
}

// Shown before the print preview (File > Stampa) - matches the reference
// FidoCadJ editor's own separate print-options dialog. Every checkbox/spinner
// here is read by MainWindow::renderForPrint() while painting, none of it
// mutates the drawing itself. Options (except which specific layer, since a
// Layer* isn't stable across sessions) persist across sessions.
class DialogPrintOptions : public QDialog
{
    Q_OBJECT

public:
    explicit DialogPrintOptions(QWidget *parent = nullptr);
    ~DialogPrintOptions();

    bool mirror() const;
    bool blackWhite() const;
    bool landscape() const;
    // Real-scale printing: when true, the drawing is printed at
    // scalePercent() of its physical size (one logical unit = 1/200 inch,
    // FidoCadJ's fixed unit) instead of being scaled to fit the page -
    // needed e.g. for toner-transfer PCB etching, where 100% must come out
    // of the printer dimensionally exact.
    bool realScale() const;
    double scalePercent() const;
    // Centimeters (top, bottom, left, right), in that order - matches the
    // "Margini (cm)" spin boxes.
    void margins(double *top, double *bottom, double *left, double *right) const;
    // Non-null only when "print only this layer" is checked.
    Layer *singleLayer() const;

    void accept() override;

private:
    void loadSavedOptions();

    Ui::DialogPrintOptions *ui;
};

#endif // DIALOGPRINTOPTIONS_H
