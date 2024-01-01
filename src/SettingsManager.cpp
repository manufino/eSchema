// SettingsManager.cpp

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
    //saveSetting("stylesheet_path", "");
    //saveSetting("lib_path", "");

    // GRIGLIA
    saveSetting("grid_step", 10);
    saveSetting("mm_step", 1.27);
    saveSetting("grid_type", "LINEE+PUNTI");
    saveSetting("grid_step_mark", 50);
    saveSetting("grid_line_width", 0.20);
    saveSetting("grid_line_mark_width", 0.20);

    // COLORI
    saveSetting("background_color", QColor("white").name());
    saveSetting("grid_dot_color", QColor("blue").name());
    saveSetting("grid_line_color", QColor(100, 100, 100).name());
    saveSetting("grid_line_mark_color", QColor(100, 100, 100).name());
}
