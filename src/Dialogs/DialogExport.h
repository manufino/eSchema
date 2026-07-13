/*
 * DialogExport.h
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

#ifndef DIALOGEXPORT_H
#define DIALOGEXPORT_H

#include <QDialog>
#include "GraphicExporter.h"

namespace Ui {
class DialogExport;
}

// File > Esporta...: one dialog for every graphic export format, collecting
// the format and its options (sizing mode, antialiasing, black & white,
// split layers) - the equivalent of the reference FidoCadJ editor's
// DialogExport. The output path is picked afterwards by the caller with a
// normal file dialog (see fileFilter()/defaultSuffix()); the export itself
// runs through the shared GraphicExporter, the same engine the command line
// -c option uses. The last-used options persist across sessions.
class DialogExport : public QDialog
{
    Q_OBJECT

public:
    // `drawingSize` (logical units) feeds the live resulting-pixel-size
    // label under the resolution spinner.
    explicit DialogExport(const QSizeF &drawingSize, QWidget *parent = nullptr);
    ~DialogExport();

    GraphicExporter::Options options() const;
    // The chosen format's file-dialog filter, e.g. "PNG file (*.png)".
    QString fileFilter() const;
    // The chosen format's canonical extension without the dot, e.g. "png".
    QString defaultSuffix() const;

    // QDialog::accept() plus persisting the chosen options for next time.
    void accept() override;

private:
    QString selectedFormat() const;
    // Enables/disables the option widgets by what the chosen format supports:
    // antialiasing is bitmap-only, sizing is meaningless for DXF (DxfWriter
    // writes logical coordinates), split-layers is unsupported for DXF.
    void updateWidgetStates();
    void updateResultLabel();
    void loadSavedOptions();

    Ui::DialogExport *ui;
    QSizeF m_drawingSize;
};

#endif // DIALOGEXPORT_H
