/*
 * DialogSymbolWizard.h
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

#ifndef DIALOGSYMBOLWIZARD_H
#define DIALOGSYMBOLWIZARD_H

#include <QDialog>
#include <QList>
#include <QString>

class GraphicsPrimitive;

namespace Ui {
class DialogSymbolWizard;
}

// Tools > Symbol wizard: generates a ready-made package footprint - DIP/
// dual-row, SIP/single-row, or quad (QFP-style) - from a handful of
// parameters (pin count, pitch, row spacing, pad/hole size, optional pin
// numbers), with a live preview, and saves it as a macro into a user
// library. The generated symbol is ordinary primitives (pads, a body
// rectangle, a pin-1 marker, text), so the .fcl output is fully
// FidoCadJ-compatible. Pin 1 is the square pad, numbering follows the
// usual counterclockwise convention.
class DialogSymbolWizard : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSymbolWizard(QWidget *parent = nullptr);
    ~DialogSymbolWizard();

    // Freshly-built primitives for the current parameters, in library-local
    // coordinates (not yet anchored at the (100,100) macro origin - the
    // caller re-anchors before serializing). Caller owns them.
    QList<GraphicsPrimitive *> buildPrimitives() const;

    QString macroName() const;
    QString category() const;
    // The chosen (or newly typed) user library's display name and its
    // on-disk filename prefix - the filename is derived from the display
    // name when the library doesn't exist yet.
    QString libraryDisplayName() const;
    QString libraryFilename() const;

private:
    void refreshPreview();
    void syncTypeDependentFields();

    Ui::DialogSymbolWizard *ui;
};

#endif // DIALOGSYMBOLWIZARD_H
