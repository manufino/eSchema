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

class Layer
{
public:

    Layer() = default;
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
    inline QColor color() const { return m_color; }
    inline QString name() const { return m_name; }
    inline bool isVisible() const { return m_visible; }
    inline bool isMaster() const { return m_master; }

    // SET
    inline void setColor(QColor color) { this->m_color = color; }
    inline void setVisible(bool visible) { this->m_visible = visible; }
    inline void setName(QString name) { this->m_name = name; }
    inline void setMaster(bool isMaster) { m_master = isMaster; }
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
};

#endif // LAYER_H
