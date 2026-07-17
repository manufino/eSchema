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

// The drawing-tools toolbar: enforces radio-button behavior across its
// checkable tool actions (exactly one checked at a time, and re-clicking
// the checked one keeps it checked instead of leaving no tool at all).
// PrimitivePlacementController reads GetCurrent() to know the active tool
// and listens to actionTriggered() for tool switches.
class ToolBarPrimitive : public QToolBar
{

public:
    ToolBarPrimitive(QWidget *parent = nullptr);

    // The currently checked tool action (never nullptr once the toolbar is
    // populated - Select is checked by default).
    QAction *GetCurrent() { return lastChecked; }

private slots:
    // Keeps the radio invariant on every click: unchecks the others and
    // remembers the newly checked action.
    void handleActionTriggered(QAction *action);


private:
    // Unchecks every checkable action except `selectedAction`.
    void uncheckOtherActions(QAction *selectedAction);

    QAction *lastChecked = nullptr;

};

#endif // TOOLBARPRIMITIVE_H
