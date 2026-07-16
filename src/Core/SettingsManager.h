/*
 * SettingsManager.h
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

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QCoreApplication>
#include <QSettings>
#include <QColor>

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager& getInstance();  // Method to get the singleton instance

    void saveSetting(const QString& key, const QVariant& value);
    QVariant loadSetting(const QString& key) const;
    void restoreDefaultSettings();

signals:
    // Emitted once per event-loop turn, no matter how many settings were
    // saved in that turn (see saveSetting()'s coalescing) - receivers are
    // whole-state refreshers (applyLiveSettings, SheetView::settingChanged,
    // ...) that would otherwise re-run dozens of times for one Options
    // dialog Apply, visibly freezing the UI.
    void settingIsChanged();

private:
    SettingsManager();  // Private constructor to prevent creating external instances
    SettingsManager(const SettingsManager&) = delete;  // Disable the copy constructor
    SettingsManager& operator=(const SettingsManager&) = delete;  // Disable the assignment operator

    QSettings m_settings;
    bool m_changeSignalPending = false; // a coalesced emission is queued
};

#endif // SETTINGS_MANAGER_H
