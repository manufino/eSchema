/*
 * statusbar.cpp
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

#include "StatusBar.h"


StatusBar::StatusBar(QWidget *parent):QStatusBar(parent)
{
    lblPos->setText(tr("X 0  Y 0 (X 0.0mm Y 0.0mm)"));

    lblPos->setMinimumWidth(350);

    lblZoomLevel->setText(tr("Zoom 7%"));
    lblPrimitiveCount->setText(tr("Primitive 0  Macro 0"));
    lblPrimitiveCount->setMinimumWidth(150);

    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &StatusBar::settingChanged);

    loadSettings();

    this->addPermanentWidget(lblZoomLevel);
    this->addPermanentWidget(lblPrimitiveCount);
    this->addPermanentWidget(lblPos);
}

void StatusBar::loadSettings()
{
    QVariant val = SettingsManager::getInstance().loadSetting("grid_step");
    gridSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("mm_step");
    mm_step = val.toDouble();
}

void StatusBar::sceneMousePos(QPointF point)
{
    int x = static_cast<int>(point.x());
    int y = static_cast<int>(point.y());

    int halfGridSize = gridSize / 2;

    // Round to the grid based on the distance from the grid line
    x = (x + halfGridSize) / gridSize * gridSize;
    y = (y + halfGridSize) / gridSize * gridSize;

    double xmm = mm_step * (x/gridSize);
    double ymm = mm_step * (y/gridSize);

    QString pos = tr("X %1 Y %2 (X %3mm Y %4mm)")
                      .arg(x).arg(y).arg(xmm).arg(ymm);
    lblPos->setText(pos);
}

void StatusBar::zoomLevel(unsigned int level)
{
    QString pos = tr("Zoom %1%").arg(level);
    lblZoomLevel->setText(pos);
}

void StatusBar::settingChanged()
{
    loadSettings();
    update();
}

void StatusBar::primitiveCounts(int totalPrimitives, int macroCount)
{
    lblPrimitiveCount->setText(tr("Primitive %1  Macro %2").arg(totalPrimitives).arg(macroCount));
}

