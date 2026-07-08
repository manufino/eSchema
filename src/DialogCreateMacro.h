/*
 * DialogCreateMacro.h
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

#ifndef DIALOGCREATEMACRO_H
#define DIALOGCREATEMACRO_H

#include <QDialog>
#include <QString>

namespace Ui {
class DialogCreateMacro;
}

// Lets the user turn the current selection into a new macro saved to a user
// (non-standard) library - "Crea macro dalla selezione" (Edit menu). Only
// collects and validates the key/name/category/library choice; building the
// macro body from the actual selected primitives happens in MainWindow,
// which has access to the canvas.
class DialogCreateMacro : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateMacro(QWidget *parent = nullptr);
    ~DialogCreateMacro();

    QString key() const;
    QString macroName() const;
    QString category() const;
    // The combo box's raw text - a library display name, either an existing
    // one or one to be created.
    QString libraryDisplayName() const;
    // The library's filename (prefix) to save into: an existing user
    // library's real filename if libraryDisplayName() matches one exactly,
    // otherwise a filesystem/FCD-prefix-safe name derived from it (a new
    // library to be created). Empty only if libraryDisplayName() has no
    // letters/digits/underscores at all.
    QString libraryFilename() const;

private slots:
    // Validates every field against LibraryManager (key well-formed, target
    // not a standard library, key not already used there) before actually
    // closing the dialog - showing labelError and staying open otherwise.
    void validateAndAccept();

private:
    Ui::DialogCreateMacro *ui;
};

#endif // DIALOGCREATEMACRO_H
