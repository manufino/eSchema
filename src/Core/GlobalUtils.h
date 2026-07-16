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

class Utils {
public:
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

    QColor randColor()
    {
        int red = QRandomGenerator::global()->bounded(256);
        int green = QRandomGenerator::global()->bounded(256);
        int blue = QRandomGenerator::global()->bounded(256);

        return QColor(red, green, blue);
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
