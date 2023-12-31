// settings_manager.cpp

#include "settings_manager.h"

SettingsManager::SettingsManager()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "eSchema", "eSchema")
{
}

void SettingsManager::saveSetting(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
    emit settingChanged(key, value);
}

QVariant SettingsManager::loadSetting(const QString& key, const QVariant& defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

void SettingsManager::restoreDefaultSettings()
{
    m_settings.clear(); // Cancella le impostazioni esistenti
    m_settings.sync(); // Assicura che le modifiche siano salvate su disco

    // Imposta le impostazioni predefinite
    saveSetting("background_color", "black");
    saveSetting("grid_color", "black");
    saveSetting("theme", "light");
}
