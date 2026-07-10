/*
 * ThemeManager.h
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

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

// Applies the app-wide QSS + QPalette selected by the "gui_style"/
// "stylesheet_path" settings (see SettingsManager and DialogOptions'
// interface-style combo box).
class ThemeManager
{
public:
    static void apply();
};

#endif // THEME_MANAGER_H
