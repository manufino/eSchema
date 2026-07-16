/*
 * DialogShortcuts.h
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

#ifndef DIALOGSHORTCUTS_H
#define DIALOGSHORTCUTS_H

#include <QDialog>
#include <QKeySequence>
#include <QList>
#include <QString>

class QAction;
class QTreeWidgetItem;

namespace Ui {
class DialogShortcuts;
}

// The keyboard-shortcut editor (Help > Keyboard shortcuts): every command,
// grouped by the menu/toolbar it lives in, with its current shortcut -
// searchable, and each one reassignable via a key-capture field. Edits
// apply immediately to the live QAction and persist across sessions
// ("shortcut_custom_<objectName>", read back by MainWindow at startup);
// assigning a sequence already used by another command offers to take it
// over. "Restore defaults" puts every command back to its shipped shortcut.
class DialogShortcuts : public QDialog
{
    Q_OBJECT

public:
    struct Command {
        QString category;          // user-visible menu/toolbar name
        QAction *action;
        QKeySequence defaultShortcut; // as shipped, before any customization
    };

    explicit DialogShortcuts(const QList<Command> &commands, QWidget *parent = nullptr);
    ~DialogShortcuts();

private:
    void filterTree(const QString &needle);
    void syncEditorToSelection();
    void assignCurrent();
    void clearCurrent();
    void restoreAllDefaults();
    // The catalog row backing `item` - -1 for category rows.
    int commandIndexOf(QTreeWidgetItem *item) const;
    // Applies `sequence` to `commands[index]`'s action, persists it, and
    // refreshes the row's Shortcut column.
    void applyShortcut(int index, const QKeySequence &sequence);

    Ui::DialogShortcuts *ui;
    QList<Command> m_commands;
    QList<QTreeWidgetItem *> m_commandItems; // parallel to m_commands
};

#endif // DIALOGSHORTCUTS_H
