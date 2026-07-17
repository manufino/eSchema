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


// Process-wide registry of the drawing layers, and the hub every layer
// change flows through. It is a singleton: the file readers/writers
// (FidoCadReader/Writer, DxfReader, GraphicExporter), every layer widget
// (LayerComboBox, LayerToolBarWidget, LayerListView, DialogLayerList) and
// each new GraphicsPrimitive (which asks for the master layer) all talk to
// the same instance. With multiple documents open, the per-document layer
// configuration is snapshotted/restored onto this same roster by
// Document::captureLayerState()/applyLayerState() on every document switch,
// so the Layer* pointers held by primitives and widgets stay valid.
class LayerList : public QObject
{
    Q_OBJECT

public:
 // Method to get the singleton instance
    static LayerList& getInstance() {
        static LayerList instance;
        return instance;
    }

    // Direct access to the live, ordered layer roster. Callers mutating it
    // (or the Layers inside) must trigger the appropriate signal themselves
    // via update()/notifyAppearanceChanged().
    QList<Layer*>*getList() { return layerList; }
    // Populates FidoCadJ's 16 default layers and colors (FIDOSPECS.md 3.1) -
    // idempotent (does nothing once any layer exists), so the GUI startup
    // and the headless command line path can both call it safely. Populating
    // all 16 (rather than just a master layer) matters for FidoCadJ
    // round-trip fidelity: a file referencing layer index 2 must land on
    // that layer, not silently collapse to layer 0 for want of one existing.
    void createDefaultLayers();
    // Appends an existing Layer (takes ownership) and fires layerListChanged.
    void addLayer(Layer *layer);
    // Convenience overload: creates the Layer from name+color, then appends.
    void addLayer(QString name, QColor color);
    // The active drawing layer (the one flagged master) - new primitives are
    // created on it. Falls back to the first layer if none is flagged.
    Layer* getMaster();
    // Reorder `layer` one position towards the front/back of the roster and
    // fire layerListChanged (used by DialogLayerList's up/down buttons).
    void moveUp(Layer *layer);
    void moveDown(Layer *layer);
    // Re-emits layerListChanged with the current roster - the "something
    // changed, rebuild yourselves" broadcast for callers that edited layers
    // directly (rename, recolor, per-document state restore).
    void update();
    // Deletes `layer` from the roster, announcing it first through
    // layerAboutToBeRemoved so primitives/widgets can drop their pointers.
    void removeLayer(Layer *layer);

public slots:
    // The three overloads all flag exactly one layer as master (clearing the
    // flag on every other), then fire layerListChanged; by object, by
    // user-visible name, or by roster index (used when restoring documents).
    void setMaster(Layer *layer);
    void setMaster(QString name);
    void setMaster(int index);
    // Shows/hides one layer and fires layerAppearanceChanged; refuses to
    // hide the master layer (you would be drawing invisibly).
    void setVisible(Layer *layer, bool visible);
    // Bulk visibility for every layer at once (master stays visible when
    // hiding) - the "show all"/"hide all" buttons; one signal for the batch.
    void setAllVisibleOrHidden(bool visible);
    // Refuses to lock the master layer (same guard shape as setVisible()'s
    // "can't hide master") - eSchema's own choice, not FidoCadJ's (which
    // doesn't restrict locking master), to avoid locking oneself out of the
    // layer actively being drawn on.
    void setLocked(Layer *layer, bool locked);
    // Bulk lock/unlock for every layer at once (master stays unlocked when
    // locking, same guard as setLocked) - one signal for the batch.
    void setAllLockedOrUnlocked(bool locked);
    // For callers that changed several layers' visible/locked flags directly
    // on the Layer objects (Document::applyLayerState() restoring a whole
    // per-document snapshot) - one appearance resync for the whole batch
    // instead of one signal per property setter.
    void notifyAppearanceChanged() { emit layerAppearanceChanged(); }

signals:
    // Structural change (add/remove/reorder/rename/master change) - heavy
    // listeners like the toolbar LayerComboBox rebuild themselves entirely.
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
