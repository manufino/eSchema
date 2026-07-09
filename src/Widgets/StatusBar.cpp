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
#include <QSignalBlocker>


StatusBar::StatusBar(QWidget *parent):QStatusBar(parent)
{
    lblPos->setText("X 0  Y 0 (X 0.0mm Y 0.0mm)");

    lblPos->setMinimumWidth(350);

    btnGrid->setCheckable(true);
    QIcon icon(":/res/resources/remix/grid-line.png");
    btnGrid->setIcon(icon);
    btnGrid->setIconSize(QSize(24,24));
    btnGrid->setMinimumSize(QSize(24,24));
    btnGrid->setMaximumSize(QSize(24,24));
    btnGrid->setToolTip("Attiva o disattiva la griglia");
    // Persists across restarts (see loadSettings()'s own read of the same
    // key) - toggled() only fires on an actual state change, so the initial
    // setChecked() below can't be relied on to also save it back.
    connect(btnGrid, &QPushButton::toggled, this, [](bool checked) {
        SettingsManager::getInstance().saveSetting("grid_visible", checked);
    });

    btnSnapGrid->setCheckable(true);
    QIcon icon2(":/res/resources/remix/grid-fill.png");
    btnSnapGrid->setIcon(icon2);
    btnSnapGrid->setIconSize(QSize(24,24));
    btnSnapGrid->setMinimumSize(QSize(24,24));
    btnSnapGrid->setMaximumSize(QSize(24,24));
    btnSnapGrid->setToolTip("Attiva o disattiva \nil snap sulla griglia");
    // Same "snap_enabled" key the Options dialog's own checkbox reads/writes
    // (GlobalUtils.h's snapToGrid() is what actually consults it) - this
    // button is just a second, quicker way to flip the very same setting.
    connect(btnSnapGrid, &QPushButton::toggled, this, [](bool checked) {
        SettingsManager::getInstance().saveSetting("snap_enabled", checked);
    });

    lblZoomLevel->setText("Zoom 7%");
    lblPrimitiveCount->setText("Primitive 0  Macro 0");
    lblPrimitiveCount->setMinimumWidth(150);

    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &StatusBar::settingChanged);

    loadSettings();

    // Reflects whatever was last saved (or the compiled-in default, for a
    // setting file saved before this button persisted anything) rather than
    // always starting checked, now that both buttons actually persist.
    // Signals blocked: this is loading the saved state, not the user
    // changing it, so it must not immediately re-trigger a save.
    QVariant gridVisible = SettingsManager::getInstance().loadSetting("grid_visible");
    const QSignalBlocker blockGrid(btnGrid);
    btnGrid->setChecked(gridVisible.isValid() ? gridVisible.toBool() : true);
    QVariant snapEnabled = SettingsManager::getInstance().loadSetting("snap_enabled");
    const QSignalBlocker blockSnap(btnSnapGrid);
    btnSnapGrid->setChecked(snapEnabled.isValid() ? snapEnabled.toBool() : true);

    this->addPermanentWidget(lblZoomLevel);
    this->addPermanentWidget(lblPrimitiveCount);
    this->addPermanentWidget(lblPos);
    this->addPermanentWidget(btnGrid);
    this->addPermanentWidget(btnSnapGrid);
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

    QString pos = QString("X %1 Y %2 (X %3mm Y %4mm)")
                      .arg(x).arg(y).arg(xmm).arg(ymm);
    lblPos->setText(pos);
}

void StatusBar::zoomLevel(unsigned int level)
{
    QString pos = QString("Zoom %1%").arg(level);
    lblZoomLevel->setText(pos);
}

void StatusBar::settingChanged()
{
    loadSettings();
    update();
}

void StatusBar::primitiveCounts(int totalPrimitives, int macroCount)
{
    lblPrimitiveCount->setText(QString("Primitive %1  Macro %2").arg(totalPrimitives).arg(macroCount));
}

