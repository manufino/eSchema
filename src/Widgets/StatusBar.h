/*
 * statusbar.h
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

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include "SettingsManager.h"

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);
    QPushButton *btnGrid = new QPushButton(this);
    QPushButton *btnSnapGrid = new QPushButton(this);

private:
    void loadSettings();

public slots:
    void sceneMousePos(QPointF point);
    void zoomLevel(unsigned int level);
    void settingChanged();
    // totalPrimitives includes macros - macroCount is the subset of those
    // that are PartLib (MC) instances, shown separately.
    void primitiveCounts(int totalPrimitives, int macroCount);

signals:
    void zoomChanged(unsigned int level);

private:
        QLabel *lblPos = new QLabel(this);
        QLabel *lblZoomLevel = new QLabel(this);
        QLabel *lblPrimitiveCount = new QLabel(this);
        int gridSize;
        double mm_step;

};

#endif // STATUSBAR_H
