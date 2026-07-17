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
#include <QAction>
#include <QAbstractButton>
#include <QPixmap>
#include <QPainter>
#include <QHash>
#include <functional>

namespace {

// The light color dark themes re-tint monochrome icons to.
const QColor kDarkIconColor(220, 220, 220);

// Recolors an icon's neutral (unsaturated - black/gray) pixels to `color`
// while keeping their alpha (shape) untouched - turns the app's black line
// icons light for the dark themes. Saturated pixels are deliberately left
// alone, so the red control-point squares on the drawing-tool icons stay
// red on every theme instead of being flattened into the tint color.
QPixmap tintPixmap(const QPixmap &source, const QColor &color)
{
    QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            if (alpha == 0)
                continue;
            const int red = qRed(pixel), green = qGreen(pixel), blue = qBlue(pixel);
            const int spread = qMax(red, qMax(green, blue)) - qMin(red, qMin(green, blue));
            if (spread > 40)
                continue; // a colored pixel (e.g. a red handle) - keep it
            line[x] = qRgba(color.red(), color.green(), color.blue(), alpha);
        }
    }

    QPixmap result = QPixmap::fromImage(image);
    result.setDevicePixelRatio(source.devicePixelRatio());
    return result;
}

QIcon tintIcon(const QIcon &icon, const QColor &color)
{
    if (icon.isNull())
        return icon;

    QIcon result;
    const QList<QSize> sizes = icon.availableSizes();
    for (const QSize &size : sizes)
        result.addPixmap(tintPixmap(icon.pixmap(size), color));
    return result;
}

// Every QAction/QAbstractButton this app ever hands an icon to, keyed by
// pointer, so a theme switch can always re-tint from the untouched
// Designer-authored icon rather than compounding tints on top of an
// already-recolored one. Entries for destroyed objects are simply never
// looked up again - MainWindow and its actions live for the whole app
// run, so this never grows unbounded in practice.
QHash<QObject *, QIcon> &originalIcons()
{
    static QHash<QObject *, QIcon> icons;
    return icons;
}

void retintIcon(QObject *owner, const QIcon &currentIcon, bool dark,
                 const std::function<void(const QIcon &)> &setIcon)
{
    if (currentIcon.isNull() && !originalIcons().contains(owner))
        return;

    auto it = originalIcons().find(owner);
    if (it == originalIcons().end())
        it = originalIcons().insert(owner, currentIcon);

    setIcon(dark ? tintIcon(it.value(), kDarkIconColor) : it.value());
}

// Swaps every QAction/QAbstractButton icon under `root` between its
// original (light/black) version and a white-tinted one.
void retintWidgetIcons(QWidget *root, bool dark)
{
    const QList<QAction *> actions = root->findChildren<QAction *>();
    for (QAction *action : actions) {
        retintIcon(action, action->icon(), dark,
                   [action](const QIcon &icon) { action->setIcon(icon); });
    }

    const QList<QAbstractButton *> buttons = root->findChildren<QAbstractButton *>();
    for (QAbstractButton *button : buttons) {
        retintIcon(button, button->icon(), dark,
                   [button](const QIcon &icon) { button->setIcon(icon); });
    }
}

// Walks every currently-alive top-level window (MainWindow plus any open
// dialogs).
void retintIcons(bool dark)
{
    const QList<QWidget *> topLevels = qApp->topLevelWidgets();
    for (QWidget *topLevel : topLevels)
        retintWidgetIcons(topLevel, dark);
}

// RulerWidget/LayerComboBox/PenStyleComboBox read palette() directly in
// their paint code rather than relying purely on the QSS cascade, so every
// built-in theme needs a matching QPalette alongside its .qss to stay
// visually consistent. This builds one from a theme's few key colors.
QPalette themedPalette(const QColor &window, const QColor &base, const QColor &text,
                       const QColor &button, const QColor &highlight,
                       const QColor &highlightedText)
{
    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::ToolTipBase, button);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, highlight);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, highlightedText);
    // Disabled text: halfway between the surface and the normal text, so it
    // reads as dimmed on any theme, light or dark.
    const QColor disabled((window.red() + text.red()) / 2,
                          (window.green() + text.green()) / 2,
                          (window.blue() + text.blue()) / 2);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    return palette;
}

QPalette darkPalette()
{
    QPalette palette = themedPalette(QColor(43, 43, 43), QColor(35, 35, 35),
                                     QColor(220, 220, 220), QColor(58, 58, 58),
                                     QColor(47, 111, 122), Qt::white);
    palette.setColor(QPalette::Link, QColor(63, 167, 180));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(110, 110, 110));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(110, 110, 110));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(110, 110, 110));
    return palette;
}

} // namespace

bool ThemeManager::darkThemeActive()
{
    const QString style = SettingsManager::getInstance().loadSetting("gui_style").toString();
    // Must mirror apply()'s branching exactly: an empty/unrecognized value
    // falls through to Nord (dark) there, so it must count as dark here too,
    // or paint-time icons (layer eye/lock glyphs, library tree previews)
    // come out black on a dark surface. Naming the light styles keeps a
    // future dark theme from silently reintroducing that mismatch.
    return style != "light" && style != "sepia" && style != "ocean"
            && style != "system" && style != "custom";
}

QPixmap ThemeManager::themedIcon(const QPixmap &source)
{
    return darkThemeActive() ? tintPixmap(source, kDarkIconColor) : source;
}

void ThemeManager::applyToWidget(QWidget *widget)
{
    retintWidgetIcons(widget, darkThemeActive());
}

void ThemeManager::apply()
{
    // Captured on the very first call, before any setPalette() below ever
    // runs (apply() is invoked once at the top of main(), prior to any
    // theme switch) - this is the only way to recover the true OS-default
    // palette later for the light/system/custom cases.
    static const QPalette kDefaultPalette = qApp->palette();

    const QString style = SettingsManager::getInstance().loadSetting("gui_style").toString();

    // Dark surfaces need the monochrome black icons re-tinted light.
    bool darkIcons = false;

    QString qssPath;
    if (style == "light") {
        // An explicit choice only - the shipped default (empty/unrecognized
        // value, i.e. a first launch) falls through to Nord below.
        // An explicit light palette rather than kDefaultPalette: the OS
        // default is only light on Windows - on a Linux desktop running a
        // dark color scheme it is dark, which turned every natively-painted
        // surface (toolbar areas, rulers, layer combo) black under the
        // light stylesheet. The colors replicate the classic Windows
        // palette this theme was designed on.
        qssPath = ":/styles/eSchema.qss";
        qApp->setPalette(themedPalette(QColor(0xf0, 0xf0, 0xf0), Qt::white,
                                       Qt::black, QColor(0xf0, 0xf0, 0xf0),
                                       QColor(0x00, 0x78, 0xd7), Qt::white));
    } else if (style == "dark") {
        qssPath = ":/styles/eSchema_dark.qss";
        qApp->setPalette(darkPalette());
        darkIcons = true;
    } else if (style == "midnight") {
        // Deep navy with violet-blue accents, Trebuchet MS.
        qssPath = ":/styles/eSchema_midnight.qss";
        qApp->setPalette(themedPalette(QColor(0x14, 0x1a, 0x2e), QColor(0x0f, 0x14, 0x25),
                                       QColor(0xc7, 0xd0, 0xf0), QColor(0x1e, 0x27, 0x42),
                                       QColor(0x3d, 0x59, 0xa1), Qt::white));
        darkIcons = true;
    } else if (style == "graphite") {
        // Neutral grays with an orange accent, Consolas.
        qssPath = ":/styles/eSchema_graphite.qss";
        qApp->setPalette(themedPalette(QColor(0x26, 0x26, 0x26), QColor(0x1f, 0x1f, 0x1f),
                                       QColor(0xe0, 0xe0, 0xe0), QColor(0x38, 0x38, 0x38),
                                       QColor(0xb3, 0x5a, 0x12), Qt::white));
        darkIcons = true;
    } else if (style == "sepia") {
        // Warm paper tones, Georgia.
        qssPath = ":/styles/eSchema_sepia.qss";
        qApp->setPalette(themedPalette(QColor(0xf4, 0xec, 0xd8), QColor(0xfd, 0xf8, 0xec),
                                       QColor(0x4a, 0x3b, 0x24), QColor(0xef, 0xe4, 0xc8),
                                       QColor(0xc8, 0xa8, 0x6e), QColor(0x2e, 0x24, 0x13)));
    } else if (style == "ocean") {
        // Fresh light blue-teal, Verdana.
        qssPath = ":/styles/eSchema_ocean.qss";
        qApp->setPalette(themedPalette(QColor(0xea, 0xf4, 0xf8), QColor(0xff, 0xff, 0xff),
                                       QColor(0x14, 0x3a, 0x4d), QColor(0xdc, 0xec, 0xf4),
                                       QColor(0x7f, 0xc3, 0xd8), QColor(0x06, 0x23, 0x30)));
    } else if (style == "system") {
        qssPath = ":/styles/eSchema_system.qss";
        qApp->setPalette(kDefaultPalette);
    } else if (style == "custom") {
        // Arbitrary user-supplied .qss file, chosen via DialogOptions'
        // "Percorso stylesheet" browse button.
        qssPath = SettingsManager::getInstance().loadSetting("stylesheet_path").toString();
        qApp->setPalette(kDefaultPalette);
    } else {
        // "nord", or any missing/unrecognized value: arctic blue-gray (the
        // Nord palette), Segoe UI - the default look on a fresh install.
        qssPath = ":/styles/eSchema_nord.qss";
        qApp->setPalette(themedPalette(QColor(0x2e, 0x34, 0x40), QColor(0x27, 0x2c, 0x36),
                                       QColor(0xd8, 0xde, 0xe9), QColor(0x3b, 0x42, 0x52),
                                       QColor(0x5e, 0x81, 0xac), Qt::white));
        darkIcons = true;
    }

    QString qss;
    if (!qssPath.isEmpty()) {
        QFile file(qssPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qss = QString::fromUtf8(file.readAll());
        }
    }
    qApp->setStyleSheet(qss);

    retintIcons(darkIcons);
}
