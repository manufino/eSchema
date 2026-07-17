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
// (non-standard) library - the new "create macro from selection" Edit-menu
// action. Only collects and validates the name/category/library choice - the
// macro's key is generated automatically (see MainWindow::
// clickCreateMacroAction()), and building the macro body from the actual
// selected primitives happens there too, since only MainWindow has access
// to the canvas.
class DialogCreateMacro : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateMacro(QWidget *parent = nullptr);
    ~DialogCreateMacro();

    // The user-entered display name and category for the new macro.
    QString macroName() const;
    QString category() const;
    // The chosen library's display name - either an existing user library's,
    // or one just created via the combo's "new library" entry.
    QString libraryDisplayName() const;
    // The library's filename (prefix) to save into: an existing user
    // library's real filename if libraryDisplayName() matches one exactly,
    // otherwise a filesystem/FCD-prefix-safe name derived from it (a new
    // library to be created). Empty only if libraryDisplayName() has no
    // letters/digits/underscores at all.
    QString libraryFilename() const;

private slots:
    // Reacts only to the user actually picking an item (not to the combo
    // being populated programmatically): offers to name a new library when
    // the trailing "new library" entry is chosen, inserting and selecting it
    // in the list, or reverts the selection if the user cancels that prompt.
    void handleLibrarySelected(int index);
    // Validates every field against LibraryManager (target not a standard
    // library, a library is actually selected) before actually closing the
    // dialog - showing labelError and staying open otherwise.
    void validateAndAccept();

private:
    Ui::DialogCreateMacro *ui;
    int m_previousLibraryIndex = -1;
};

#endif // DIALOGCREATEMACRO_H
