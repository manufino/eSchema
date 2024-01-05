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
