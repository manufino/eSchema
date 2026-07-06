/*
 * toolbarprimitive.h
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

#ifndef TOOLBARPRIMITIVE_H
#define TOOLBARPRIMITIVE_H

#include <QToolBar>

class ToolBarPrimitive : public QToolBar
{

public:
    ToolBarPrimitive(QWidget *parent = nullptr);

    QAction *GetCurrent() { return lastChecked; }

private slots:
    void handleActionTriggered(QAction *action);


private:
    void uncheckOtherActions(QAction *selectedAction);

    QAction *lastChecked = nullptr;

};

#endif // TOOLBARPRIMITIVE_H
