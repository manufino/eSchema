/*
 * GlobalUtils.h
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

#ifndef GLOBALUTILS_H
#define GLOBALUTILS_H

#include <QCoreApplication>
#include <QDebug>
#include <QRandomGenerator>
#include <QColor>
#include <QPointF>
#include <QString>
#include <QVariant>
#include <cmath>
#include "SettingsManager.h"

// Translatable strings needed word-for-word by more than one class: a
// single shared declaration (one entry per language in the .ts files)
// instead of every tr() context re-declaring its own identical copy.
namespace SharedTexts {

inline QString imageFileFilter()
{
    return QCoreApplication::translate("SharedTexts",
            "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
}

inline QString unableToReadFile(const QString &path)
{
    return QCoreApplication::translate("SharedTexts",
            "Unable to read the file:\n%1").arg(path);
}

} // namespace SharedTexts

// Small grab-bag of process-wide helpers with no better home: the grid
// rendering style enum, random layer colors, and the one shared
// snap-to-grid rule. Singleton so SheetView, GraphicsPrimitive and the
// dialogs all apply identical behavior.
class Utils {
public:
    // How SheetView::drawBackground() renders the grid (persisted as the
    // "grid_type" setting, chosen in Options > Drawing).
    typedef enum {
        LinesAndDots,
        Dots,
        Lines
    } GridType;

    static Utils& instance()
    {
        static Utils instance; // guarantees the single shared instance
        return instance;
    }

    // A fully random opaque color - the default for newly created layers
    // when the caller doesn't pick one (see Layer's constructor).
    QColor randColor()
    {
        int red = QRandomGenerator::global()->bounded(256);
        int green = QRandomGenerator::global()->bounded(256);
        int blue = QRandomGenerator::global()->bounded(256);

        return QColor(red, green, blue);
    }

    // The drawing area's configured background color ("background_color"
    // setting, stored as a #rrggbb name) - white until the user changes it.
    QColor canvasBackgroundColor()
    {
        const QVariant val = SettingsManager::getInstance().loadSetting("background_color");
        const QColor color(val.toString());
        return color.isValid() ? color : QColor(Qt::white);
    }

    // `color` adjusted, when needed, to stay visible against the canvas
    // background - for colors arriving from imported files (DXF/SVG), which
    // may legitimately be white-on-white: AutoCAD's color 7 means
    // "foreground" and renders black on light backgrounds there, and SVG
    // clipart is routinely drawn in white. A color already contrasting
    // enough (luminance difference >= 0.3) is returned untouched; the pure
    // background-side extreme flips to the opposite one (white -> black on
    // a light canvas, black -> white on a dark one, the AutoCAD
    // convention); anything else keeps its hue while its luminance is
    // pushed away from the background's.
    QColor contrastingDrawingColor(const QColor &color)
    {
        auto luminance = [](const QColor &c) {
            return 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();
        };
        const QColor background = canvasBackgroundColor();
        const qreal colorLum = luminance(color);
        const qreal bgLum = luminance(background);
        if (std::abs(colorLum - bgLum) >= 0.3)
            return color;
        if (bgLum >= 0.5) {
            if (colorLum >= 0.95)
                return QColor(Qt::black);
            // Scale the channels down: same hue, darker.
            const qreal target = qMax<qreal>(0.0, bgLum - 0.5);
            const qreal factor = colorLum > 0.0 ? target / colorLum : 0.0;
            return QColor::fromRgbF(color.redF() * factor, color.greenF() * factor,
                                    color.blueF() * factor);
        }
        if (colorLum <= 0.05)
            return QColor(Qt::white);
        // Blend the channels toward white: same hue, brighter.
        const qreal target = qMin<qreal>(1.0, bgLum + 0.5);
        const qreal blend = (target - colorLum) / (1.0 - colorLum);
        return QColor::fromRgbF(color.redF() + blend * (1.0 - color.redF()),
                                color.greenF() + blend * (1.0 - color.greenF()),
                                color.blueF() + blend * (1.0 - color.blueF()));
    }

    // Rounds a scene position to the nearest multiple of the configured snap
    // step ("snap_step" setting), or returns it unchanged when "snap_enabled"
    // is off. Shared by GraphicsPrimitive::itemChange (move) and
    // SheetView::snapToGrid (placement) so the rule lives in exactly one place.
    QPointF snapToGrid(const QPointF &pos)
    {
        QVariant enabledVal = SettingsManager::getInstance().loadSetting("snap_enabled");
        const bool snapEnabled = enabledVal.isValid() ? enabledVal.toBool() : true;
        if (!snapEnabled)
            return pos;

        QVariant stepVal = SettingsManager::getInstance().loadSetting("snap_step");
        // Falls back to 10 (matching the default visible grid spacing) rather
        // than 1, so snapping is actually visible at the default zoom instead
        // of rounding to an imperceptibly fine 1-unit grid.
        const int step = stepVal.isValid() && stepVal.toInt() > 0 ? stepVal.toInt() : 10;

        return QPointF(std::round(pos.x() / step) * step, std::round(pos.y() / step) * step);
    }

private:
    // Private constructor to prevent creating external instances
    Utils() {}
    ~Utils() = default;
    Utils(const Utils&) = delete;
    Utils& operator=(const Utils&) = delete;
};

#endif // GLOBALUTILS_H
