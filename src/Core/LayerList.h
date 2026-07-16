/*
 * LayerList.h
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

#ifndef LAYERLIST_H
#define LAYERLIST_H
#include "Layer.h"
#include <QObject>


class LayerList : public QObject
{
    Q_OBJECT

public:
 // Method to get the singleton instance
    static LayerList& getInstance() {
        static LayerList instance;
        return instance;
    }

    QList<Layer*>*getList() { return layerList; }
    // Populates FidoCadJ's 16 default layers and colors (FIDOSPECS.md 3.1) -
    // idempotent (does nothing once any layer exists), so the GUI startup
    // and the headless command line path can both call it safely. Populating
    // all 16 (rather than just a master layer) matters for FidoCadJ
    // round-trip fidelity: a file referencing layer index 2 must land on
    // that layer, not silently collapse to layer 0 for want of one existing.
    void createDefaultLayers();
    void addLayer(Layer *layer);
    void addLayer(QString name, QColor color);
    Layer* getMaster();
    void moveUp(Layer *layer);
    void moveDown(Layer *layer);
    void update();
    void removeLayer(Layer *layer);

public slots:
    void setMaster(Layer *layer);
    void setMaster(QString name);
    void setMaster(int index);
    void setVisible(Layer *layer, bool visible);
    void setAllVisibleOrHidden(bool visible);
    // Refuses to lock the master layer (same guard shape as setVisible()'s
    // "can't hide master") - eSchema's own choice, not FidoCadJ's (which
    // doesn't restrict locking master), to avoid locking oneself out of the
    // layer actively being drawn on.
    void setLocked(Layer *layer, bool locked);
    void setAllLockedOrUnlocked(bool locked);

signals:
    void layerListChanged(QList<Layer*> *layerList);
    // Fired by removeLayer() right before the Layer object is deleted -
    // every holder of a raw Layer* (primitives on the sheet, combo box item
    // data) must drop or reassign it now, or be left dangling.
    void layerAboutToBeRemoved(Layer *layer);
    // Fired by setVisible/setAllVisibleOrHidden/setLocked/
    // setAllLockedOrUnlocked - lighter-weight than layerListChanged (which
    // the toolbar combobox rebuilds itself entirely from), so a simple
    // eye/lock toggle never triggers a full QComboBox::clear()+repopulate.
    // Sheet listens to this to resync every primitive's on-screen
    // visibility/selectability.
    void layerAppearanceChanged();

private:
    LayerList(){};  // Private constructor to prevent creating external instances
    LayerList(const LayerList&) = delete;  // Disable the copy constructor
    LayerList& operator=(const LayerList&) = delete;  // Disable the assignment operator

    QList<Layer*> *layerList = new QList<Layer*>;
};

#endif // LAYERLIST_H
