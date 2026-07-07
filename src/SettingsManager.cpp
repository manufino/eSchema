/*
 * SettingsManager.cpp
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

#include "SettingsManager.h"

SettingsManager& SettingsManager::getInstance()
{
    static SettingsManager instance;  // Istanzia il singleton una sola volta
    return instance;
}

SettingsManager::SettingsManager()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "eSchema", "eSchema")
{
}

void SettingsManager::saveSetting(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
    emit settingIsChanged();
}

QVariant SettingsManager::loadSetting(const QString& key) const
{
    return m_settings.value(key);
}

void SettingsManager::restoreDefaultSettings()
{
    m_settings.clear(); // Cancella le impostazioni esistenti
    m_settings.sync(); // Assicura che le modifiche siano salvate su disco

    // Impostazioni predefinite

    // GENERALE
    saveSetting("language", "it");
    saveSetting("gui_style", "light");
    saveSetting("stylesheet_path", "");
    saveSetting("lib_path", "");
    saveSetting("macro_icon_size", 32);

    // GRIGLIA
    saveSetting("grid_step", 10);
    saveSetting("mm_step", 1.27);
    saveSetting("grid_type", 0);
    saveSetting("grid_step_mark", 50);
    saveSetting("grid_line_width", 0.20);
    saveSetting("grid_line_mark_width", 0.20);

    // SNAP - defaults to the same spacing as the visible grid (grid_step) so
    // clicks visibly snap to grid intersections. Any multiple of 1 keeps
    // coordinates integers, satisfying FidoCadJ's grid-unit requirement
    // (FIDOSPECS.md 3), so this can be changed independently of grid_step.
    saveSetting("snap_enabled", true);
    saveSetting("snap_step", 10);

    // COLORI
    saveSetting("background_color", QColor("white").name());
    saveSetting("grid_dot_color", QColor("blue").name());
    saveSetting("grid_line_color", QColor(100, 100, 100).name());
    saveSetting("grid_line_mark_color", QColor(100, 100, 100).name());

    m_settings.sync();
}
