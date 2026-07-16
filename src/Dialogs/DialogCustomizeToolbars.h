/*
 * DialogCustomizeToolbars.h
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

#ifndef DIALOGCUSTOMIZETOOLBARS_H
#define DIALOGCUSTOMIZETOOLBARS_H

#include <QDialog>
#include <QHash>
#include <QList>
#include <QStringList>

namespace Ui { class DialogCustomizeToolbars; }
class QAction;
class QToolBar;
class QListWidgetItem;

// The classic two-list toolbar editor: pick a toolbar, move commands
// between "available" and "toolbar commands", reorder them, sprinkle
// separators, or reset the toolbar to its shipped default. Works entirely
// on in-memory layouts (lists of action objectNames, with "separator"
// entries); nothing touches the real toolbars until the caller reads
// layouts() after an accepted exec() - see MainWindow::
// clickCustomizeToolbarsAction().
class DialogCustomizeToolbars : public QDialog
{
    Q_OBJECT

public:
    struct ToolbarEntry {
        QToolBar *toolBar;       // the real toolbar (used as the layouts() key)
        QString title;           // user-visible name for the combo box
        QStringList currentLayout;
        QStringList defaultLayout; // what "Reset toolbar" restores
    };

    // `catalog` is every command offered in the "available" list - each must
    // have a unique, non-empty objectName.
    DialogCustomizeToolbars(const QList<QAction *> &catalog,
                            const QList<ToolbarEntry> &toolbars,
                            QWidget *parent = nullptr);
    ~DialogCustomizeToolbars();

    // The edited layout of every toolbar, keyed by the toolbar itself -
    // meaningful after exec() returned Accepted.
    QHash<QToolBar *, QStringList> layouts() const;

private:
    void loadToolbar(int index);
    void storeCurrentList();       // list widget -> m_toolbars[m_currentIndex]
    void rebuildLists();
    QListWidgetItem *makeCommandItem(const QString &objectName) const;
    void moveCurrentRow(int delta);

    Ui::DialogCustomizeToolbars *ui;
    QList<QAction *> m_catalog;
    QList<ToolbarEntry> m_toolbars;
    int m_currentIndex = -1;
};

#endif // DIALOGCUSTOMIZETOOLBARS_H
