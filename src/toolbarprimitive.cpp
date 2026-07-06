/*
 * toolbarprimitive.cpp
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

#include "ToolBarPrimitive.h"

ToolBarPrimitive::ToolBarPrimitive(QWidget *parent) : QToolBar(parent)
{
     connect(this, &QToolBar::actionTriggered, this, &ToolBarPrimitive::handleActionTriggered);

     for (QAction *action : actions()) {
         if (action->isChecked()) {
             lastChecked = action;
         }
     }
}

void ToolBarPrimitive::handleActionTriggered(QAction *action)
{
    // Porta in stato unchecked tutte le altre azioni della toolbar
    uncheckOtherActions(action);
    lastChecked = action;
}

void ToolBarPrimitive::uncheckOtherActions(QAction *selectedAction)
{
    // Itera attraverso tutte le azioni della toolbar
    for (QAction *action : actions()) {
        if (action != selectedAction && action->isCheckable()) {
            action->setChecked(false);
        }
    }
}
