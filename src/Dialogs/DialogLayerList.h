/*
 * DialogLayerList.h
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

#ifndef DIALOGLAYERLIST_H
#define DIALOGLAYERLIST_H

#include <QWidget>

#include "Layer.h"
#include "LayerListView.h"

namespace Ui {
class DialogLayerList;
}

// The layer manager: a LayerListView of every layer with buttons to
// reorder, add, delete, and bulk-toggle visibility/locking. Despite the
// legacy Dialog name, this is a plain widget - it lives inside the "Layers"
// dock panel (ui->dockLayers in MainWindow.ui), so layers can be managed
// while drawing. All mutations go through the LayerList singleton, so the
// toolbar combo and the canvas stay in sync at all times.
class DialogLayerList : public QWidget
{
    Q_OBJECT

public:
    explicit DialogLayerList(QWidget *parent = nullptr);
    ~DialogLayerList();

public slots:
    // Move the selected layer one position up/down in the roster
    // (LayerList::moveUp()/moveDown()); no-ops with nothing selected.
    void levelUp();
    void levelDown();
    // Bulk toggles, one LayerList call each (master always stays visible
    // and unlocked - see LayerList's guards).
    void setAllVisible();
    void setAllHidden();
    void setAllLocked();
    void setAllUnlocked();
    // Deletes the selected layer (its primitives move to the master layer -
    // see Sheet::reassignLayerBeforeRemoval()); refuses to delete the last one.
    void deleteCurrent();
    // Appends a new layer with a unique default name and a random color.
    void addNewLayer();

private:
    // True when the list has a selected row (guards the per-layer buttons).
    bool layerIsSelected();
    QColor randomColor();


private:
    Ui::DialogLayerList *ui;
};

#endif // DIALOGLAYERLIST_H
