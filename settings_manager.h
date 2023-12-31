// settings_manager.h

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QColor>

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    SettingsManager();  // Costruttore

    void saveSetting(const QString& key, const QVariant& value);
    QVariant loadSetting(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void restoreDefaultSettings();

signals:
    void settingChanged(const QString& key, const QVariant& value);

private:
    QSettings m_settings;
};

#endif // SETTINGS_MANAGER_H
