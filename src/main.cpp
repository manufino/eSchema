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

#include "CommandLine.h"
#include "MainWindow.h"
#include "SettingsManager.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QFileInfo>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>

namespace {
// Every language eSchema ships a translation for, besides "en" (the source
// language the UI strings are written in, so it needs no QTranslator at all).
const QStringList SupportedLanguages = {
    "it", "de", "fr", "es", "ru", "zh", "pl", "ja", "pt", "ar", "hi"
};

// Reads the user's saved language choice (see DialogOptions), or - on the
// very first run, before that setting exists - derives one from the OS
// locale and persists it, so the app then keeps opening in that language
// regardless of later OS locale changes until the user picks a different one
// from Options.
QString resolveLanguageCode()
{
    const QVariant saved = SettingsManager::getInstance().loadSetting("language");
    if (saved.isValid() && !saved.toString().isEmpty())
        return saved.toString();

    QString detected = QStringLiteral("it");
    for (const QString &uiLanguage : QLocale::system().uiLanguages()) {
        const QString code = QLocale(uiLanguage).name().section('_', 0, 0);
        if (code == QStringLiteral("en") || SupportedLanguages.contains(code)) {
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

    // FidoCadJ-compatible command line (see CommandLine.h): "-h" and parse
    // errors stop here; "-c"/"-s" run headless conversions before (or, with
    // "-n", instead of) the GUI.
    CommandLine commandLine;
    const int parseResult = commandLine.process(a.arguments());
    if (parseResult >= 0)
        return parseResult;

    int headlessResult = 0;
    if (commandLine.headlessMode())
        headlessResult = commandLine.runHeadlessTasks();
    // Like FidoCadJ, the GUI still opens after a -c/-s unless -n was given.
    if (commandLine.commandLineOnly())
        return headlessResult;

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
    splash.showMessage("Loading ...");

    // The command line's -l overrides (without persisting) the saved
    // language choice, matching FidoCadJ's own -l semantics.
    QTranslator translator;
    const QString languageCode = commandLine.wantedLanguage().isEmpty()
            ? resolveLanguageCode() : commandLine.wantedLanguage();
    if (languageCode != QStringLiteral("en") && translator.load(":/i18n/eSchema_" + languageCode))
        a.installTranslator(&translator);

    MainWindow w;
    w.show();
    splash.finish(&w);

    // "eschema drawing.fcd" opens that file directly, matching FidoCadJ's
    // command-line behaviour. Otherwise, optionally (Options > General)
    // reopen whatever was worked on last.
    if (!commandLine.loadFileName().isEmpty()) {
        w.openFile(commandLine.loadFileName());
    } else if (SettingsManager::getInstance().loadSetting("startup_reopen_last").toBool()) {
        const QStringList recents =
                SettingsManager::getInstance().loadSetting("recent_files").toStringList();
        if (!recents.isEmpty() && QFileInfo::exists(recents.first()))
            w.openFile(recents.first());
    }

    return a.exec();
}
