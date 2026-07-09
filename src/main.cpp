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

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "eSchema_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

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
