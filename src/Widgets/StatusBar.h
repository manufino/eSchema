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
#include <QToolButton>
#include <QMouseEvent>

#include "SettingsManager.h"

// The main window's status bar: live cursor coordinates (grid-rounded, in
// drawing units and/or millimeters per the "units_display" setting), the
// zoom percentage, and the primitive/macro counters. Purely passive - every
// readout is fed by the active document's SheetView/Sheet through the slots
// below, so switching documents just means reconnecting the signals.
class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);

private:
    // Reads grid step, mm conversion and units preference from settings.
    void loadSettings();

public slots:
    // Updates the coordinate readout - connected to SheetView::mouseMoved.
    void sceneMousePos(QPointF point);
    // Updates the zoom readout (a clickable button, see zoomWidgetClicked())
    // - connected to SheetView::zoomScaleIsChanged.
    void zoomLevel(unsigned int level);
    // Re-reads the settings so unit/grid changes apply live.
    void settingChanged();
    // totalPrimitives includes macros - macroCount is the subset of those
    // that are PartLib (MC) instances, shown separately.
    void primitiveCounts(int totalPrimitives, int macroCount);
    // Shows "Nodes N" while a single complex curve/polygon is selected on
    // the canvas; a negative count hides the readout entirely - fed by
    // MainWindow::updatePropertiesPanel().
    void selectedNodeCount(int count);
    // Updates the snap-step readout (a clickable button, like the zoom
    // one - see snapWidgetClicked()) - fed by MainWindow whenever the
    // "snap_step" setting changes.
    void snapStep(int step);

signals:
    // Forwarded zoom-level change (kept for symmetry with zoomLevel()).
    void zoomChanged(unsigned int level);
    // The zoom readout was clicked; `globalPos` is where a popup menu
    // should open (the button's bottom-left corner - QMenu flips it above
    // by itself this close to the screen edge). MainWindow builds the menu:
    // it owns the fit actions and the active document's view.
    void zoomWidgetClicked(const QPoint &globalPos);
    // Same contract for the snap-step readout - MainWindow builds the
    // preset/custom-value menu and writes the "snap_step" setting.
    void snapWidgetClicked(const QPoint &globalPos);

private:
        QLabel *lblPos = new QLabel(this);
        // A flat button rather than a label: the zoom readout doubles as
        // the trigger for the zoom presets menu.
        // Flat clickable readouts (see zoomWidgetClicked()/snapWidgetClicked()).
        QToolButton *btnSnapStep = new QToolButton(this);
        QToolButton *btnZoomLevel = new QToolButton(this);
        // Hidden until a curve/polygon is selected - see selectedNodeCount().
        QLabel *lblNodeCount = new QLabel(this);
        QLabel *lblPrimitiveCount = new QLabel(this);
        int gridSize;
        double mm_step;
        int unitsDisplay = 0; // 0 = both, 1 = drawing units, 2 = millimeters

};

#endif // STATUSBAR_H
