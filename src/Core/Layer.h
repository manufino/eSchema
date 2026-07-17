/*
 * Layer.h
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

#ifndef LAYER_H
#define LAYER_H

#include <QObject>
#include <QWidget>
#include <QColor>
#include "GlobalUtils.h"

// One drawing layer: a name, a color, and the visible/locked/master flags.
// Plain data holder with no signals of its own - all instances live in (and
// are owned by) the process-wide LayerList singleton, which is the one that
// notifies the UI when a layer changes. Primitives reference their layer by
// raw Layer* (GraphicsPrimitive::layer()), so a Layer must never be deleted
// without LayerList::layerAboutToBeRemoved() going out first.
class Layer
{
public:

    Layer() = default;
    // `level` is the FidoCadJ layer index (0-15); `isMaster` marks the layer
    // new primitives are created on. Layers start visible and unlocked.
    Layer(QString name, QColor color = Utils::instance().randColor(),
          bool isMaster = false, int level = 0)
    {
        m_name = name;
        m_color = color;
        m_level = level;
        m_visible = true;
        m_master = isMaster;
    }
    // GET
    // The color every primitive on this layer is drawn with.
    inline QColor color() const { return m_color; }
    // User-visible layer name (shown in the layer combobox/manager).
    inline QString name() const { return m_name; }
    // Whether primitives on this layer are drawn at all - Sheet resyncs each
    // primitive's QGraphicsItem visibility from this on layerAppearanceChanged.
    inline bool isVisible() const { return m_visible; }
    // Whether this is the active drawing layer - the one new primitives are
    // assigned to (GraphicsPrimitive's constructor asks LayerList::getMaster()).
    inline bool isMaster() const { return m_master; }
    // Whether primitives on this layer can be selected/edited - matches the
    // reference FidoCadJ editor's LayerDesc.isLocked() (persisted as "FJC K"
    // in the .fcd file, see FidoCadWriter/FidoCadReader).
    inline bool isLocked() const { return m_locked; }

    // SET
    // Plain field writes - none of these notify anyone. Callers that change
    // a layer the UI can see must go through the LayerList slots (or call
    // LayerList::update()/notifyAppearanceChanged() afterwards) so the
    // widgets and sheets resync.
    inline void setColor(QColor color) { this->m_color = color; }
    inline void setVisible(bool visible) { this->m_visible = visible; }
    inline void setName(QString name) { this->m_name = name; }
    inline void setMaster(bool isMaster) { m_master = isMaster; }
    inline void setLocked(bool locked) { this->m_locked = locked; }
/*
    bool operator ==(Layer &layer) const {
        return layer.name() == m_name;
    }
*/
private:
    QColor m_color;
    QString m_name;
    int m_level;
    bool m_visible;
    bool m_master;
    // In-class default initializer (unlike m_visible/m_master above) so it's
    // safely false regardless of which constructor runs, including the
    // defaulted Layer() one that never touches the other fields either.
    bool m_locked = false;
};

#endif // LAYER_H
