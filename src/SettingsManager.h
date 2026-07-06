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
    static SettingsManager& getInstance();  // Metodo per ottenere l'istanza del singleton

    void saveSetting(const QString& key, const QVariant& value);
    QVariant loadSetting(const QString& key) const;
    void restoreDefaultSettings();

signals:
    void settingIsChanged();

private:
    SettingsManager();  // Costruttore privato per impedire la creazione di istanze esterne
    SettingsManager(const SettingsManager&) = delete;  // Disabilita il costruttore di copia
    SettingsManager& operator=(const SettingsManager&) = delete;  // Disabilita l'operatore di assegnazione

    QSettings m_settings;
};

#endif // SETTINGS_MANAGER_H
