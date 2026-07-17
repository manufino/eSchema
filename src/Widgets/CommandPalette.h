/*
 * CommandPalette.h
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

#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QDialog>
#include <QList>
#include <QString>

class QAction;
class QLineEdit;
class QListWidget;

// The VS Code-style command palette (Ctrl+Shift+P): a popup with a search
// box that filters every command in the application by name (and by the
// menu it lives in), Up/Down to pick one, Enter to run it. Commands are
// the same QAction objects the menus/toolbars use, so icons, shortcuts and
// enabled state always match - a disabled command is shown greyed out and
// can't be run. The popup closes on Escape, on a click outside, or after
// running a command.
class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    explicit CommandPalette(QWidget *parent = nullptr);

    // Registers `action` under `category` (the user-visible name of the
    // menu/toolbar it comes from - searched too, so typing "edit" narrows
    // to the Edit menu's commands).
    void addCommand(const QString &category, QAction *action);

    // Sizes the popup, centers it horizontally on `topCenter` (global
    // coordinates, its top edge landing there - clamped to stay on screen),
    // and runs it modally. `initialText` seeds the search box (used by the
    // menu bar's search field, so what was typed there carries over).
    void popup(const QPoint &topCenter, const QString &initialText = QString());

protected:
    // Forwards Up/Down/PageUp/PageDown from the search box to the list, so
    // the selection can be moved without leaving the typing focus.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // Rebuilds the visible list to the entries matching `needle` (substring
    // match on command text and category), keeping the first row selected.
    void refilter(const QString &needle);
    // Runs the selected command's QAction (if enabled) and closes the popup.
    void triggerCurrent();

    struct Entry {
        QString category;
        QAction *action;
    };

    QLineEdit *m_edit;
    QListWidget *m_list;
    QList<Entry> m_entries;
};

#endif // COMMANDPALETTE_H
