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
 // Metodo per ottenere l'istanza del singleton
    static LayerList& getInstance() {
        static LayerList instance;
        return instance;
    }

    QList<Layer*>*getList() { return layerList; }
    void setList(QList<Layer*> *list) { layerList = list; }
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

signals:
    void layerListChanged(QList<Layer*> *layerList);

private:
    LayerList(){};  // Costruttore privato per impedire la creazione di istanze esterne
    LayerList(const LayerList&) = delete;  // Disabilita il costruttore di copia
    LayerList& operator=(const LayerList&) = delete;  // Disabilita l'operatore di assegnazione

    QList<Layer*> *layerList = new QList<Layer*>;
};

#endif // LAYERLIST_H
