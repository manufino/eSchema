/*
 * main.cpp
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

#include "MainWindow.h"
#include "SettingsManager.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>

namespace {
// Every language eSchema ships a translation for, besides "it" (the source
// language the UI strings are written in, so it needs no QTranslator at all).
const QStringList SupportedLanguages = {
    "en", "de", "fr", "es", "ru", "zh", "pl", "ja", "pt", "ar", "hi"
};

// Reads the user's saved language choice (see DialogOptions), or - on the
// very first run, before that setting exists - derives one from the OS
// locale and persists it, so the app then keeps opening in that language
// regardless of later OS locale changes until the user picks a different one
// from Opzioni.
QString resolveLanguageCode()
{
    const QVariant saved = SettingsManager::getInstance().loadSetting("language");
    if (saved.isValid() && !saved.toString().isEmpty())
        return saved.toString();

    QString detected = QStringLiteral("it");
    for (const QString &uiLanguage : QLocale::system().uiLanguages()) {
        const QString code = QLocale(uiLanguage).name().section('_', 0, 0);
        if (code == QStringLiteral("it") || SupportedLanguages.contains(code)) {
            detected = code;
            break;
        }
    }
    SettingsManager::getInstance().saveSetting("language", detected);
    return detected;
}
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ThemeManager::apply();

    QPixmap pixmap(":/res/resources/splash.jpeg");

    int newWidth = 500; // desired width
    int newHeight = 0;  // height will be computed automatically to keep the aspect ratio

    // Compute the new height, keeping the original aspect ratio
    newHeight = pixmap.height() * newWidth / pixmap.width();

    // Resize the QPixmap, keeping the original aspect ratio
    QPixmap resizedPixmap = pixmap.scaled(newWidth, newHeight, Qt::KeepAspectRatio);

    QSplashScreen splash(resizedPixmap);
    splash.show();
    splash.showMessage("Caricamento ...");

    QTranslator translator;
    const QString languageCode = resolveLanguageCode();
    if (languageCode != QStringLiteral("it") && translator.load(":/i18n/eSchema_" + languageCode))
        a.installTranslator(&translator);

    MainWindow w;
    w.show();
    splash.finish(&w);

    // "eschema drawing.fcd" opens that file directly, matching FidoCadJ's
    // command-line behaviour.
    const QStringList args = a.arguments();
    if (args.size() > 1)
        w.openFile(args.at(1));

    return a.exec();
}
