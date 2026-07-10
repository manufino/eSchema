/*
 * ThemeManager.cpp
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

#include "ThemeManager.h"
#include "SettingsManager.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QColor>

namespace {

// RulerWidget/LayerComboBox/PenStyleComboBox read palette() directly in
// their paint code rather than relying purely on the QSS cascade, so the
// dark theme needs a matching QPalette alongside eSchema_dark.qss to stay
// visually consistent.
QPalette darkPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(43, 43, 43));
    palette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    palette.setColor(QPalette::Base, QColor(35, 35, 35));
    palette.setColor(QPalette::AlternateBase, QColor(43, 43, 43));
    palette.setColor(QPalette::ToolTipBase, QColor(58, 58, 58));
    palette.setColor(QPalette::ToolTipText, QColor(220, 220, 220));
    palette.setColor(QPalette::Text, QColor(220, 220, 220));
    palette.setColor(QPalette::Button, QColor(58, 58, 58));
    palette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(63, 167, 180));
    palette.setColor(QPalette::Highlight, QColor(47, 111, 122));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
    return palette;
}

} // namespace

void ThemeManager::apply()
{
    // Captured on the very first call, before any setPalette() below ever
    // runs (apply() is invoked once at the top of main(), prior to any
    // theme switch) - this is the only way to recover the true OS-default
    // palette later for the light/system/custom cases.
    static const QPalette kDefaultPalette = qApp->palette();

    const QString style = SettingsManager::getInstance().loadSetting("gui_style").toString();

    QString qssPath;
    if (style == "dark") {
        qssPath = ":/styles/eSchema_dark.qss";
        qApp->setPalette(darkPalette());
    } else if (style == "system") {
        qssPath = ":/styles/eSchema_system.qss";
        qApp->setPalette(kDefaultPalette);
    } else if (style == "custom") {
        // Arbitrary user-supplied .qss file, chosen via DialogOptions'
        // "Percorso stylesheet" browse button.
        qssPath = SettingsManager::getInstance().loadSetting("stylesheet_path").toString();
        qApp->setPalette(kDefaultPalette);
    } else {
        // "light", or any missing/unrecognized value - the original
        // built-in eSchema look.
        qssPath = ":/styles/eSchema.qss";
        qApp->setPalette(kDefaultPalette);
    }

    QString qss;
    if (!qssPath.isEmpty()) {
        QFile file(qssPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qss = QString::fromUtf8(file.readAll());
        }
    }
    qApp->setStyleSheet(qss);
}
