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
#include <QHash>
#include <QVariant>

// The application's persistent settings store, wrapping QSettings (the
// per-user .ini/registry) behind a singleton with an in-memory read cache
// and a coalesced change signal. Every configurable behavior in the app -
// grid, snap, colors, theme, autosave, recent files, pending autosave
// registry - goes through here; DialogOptions writes, everything else
// listens to settingIsChanged() and re-reads what it cares about.
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager& getInstance();  // Method to get the singleton instance

    // Persists one key (QSettings + cache) and queues a single coalesced
    // settingIsChanged() emission for the current event-loop turn.
    void saveSetting(const QString& key, const QVariant& value);
    // Reads one key through the cache; an invalid QVariant means the key has
    // never been written (callers guard with .isValid() or a qMax clamp).
    QVariant loadSetting(const QString& key) const;
    // Wipes ALL stored settings and rewrites the shipped defaults - the
    // Options dialog's "Restore defaults" button.
    void restoreDefaultSettings();
    // First-launch safety net (called from main): writes the shipped default
    // for every missing key without touching existing values, so no reader
    // ever sees an invalid variant (a raw 0 here once meant a divide-by-zero
    // crash on a pristine install).
    void ensureDefaults();

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

    // Writes the shipped default for every key not already present, without
    // touching existing values - the single source of truth for defaults,
    // shared by ensureDefaults() (fill gaps) and restoreDefaultSettings()
    // (clear first, so everything is a gap).
    void writeMissingDefaults();

    QSettings m_settings;
    bool m_changeSignalPending = false; // a coalesced emission is queued
    // In-memory read cache over QSettings: several settings are consulted
    // on every paint/mouse-move (line width, snap radii, marker colors...),
    // and going through QSettings each time (mutex + string lookup +
    // variant conversion) adds up to real per-frame cost. Writes update the
    // cache in place; restoreDefaultSettings() drops it wholesale. mutable:
    // loadSetting() is logically const.
    mutable QHash<QString, QVariant> m_cache;
};

#endif // SETTINGS_MANAGER_H
