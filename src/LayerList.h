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


    QList<Layer>* getList() { return layerList; }
    void setList(QList<Layer> *list) { layerList = list; }

    void addLayer(Layer layer) {
        layerList->append(layer);
        emit layerListChanged(layerList);
    }

    void addLayer(QString name, QColor color) {
        Layer layer(name, color);
        layerList->append(layer);
        emit layerListChanged(layerList);
    }

    Layer* getMaster() {
        for (int i = 0; i < layerList->count(); i++) {
            Layer &currentLayer = const_cast<Layer&>(layerList->at(i));
            if(currentLayer.isMaster())
                return &currentLayer;
        }
    }

public slots:
    void setMaster(Layer *layer) {
        for (int i = 0; i < layerList->count(); i++) {
            Layer &currentLayer = (*layerList)[i];
            if (&currentLayer == layer) {
                currentLayer.setMaster(true);
            } else {
                currentLayer.setMaster(false);
            }
        }
    }

signals:
    void layerListChanged(QList<Layer> *layerList);


private:
    LayerList(){};  // Costruttore privato per impedire la creazione di istanze esterne
    LayerList(const LayerList&) = delete;  // Disabilita il costruttore di copia
    LayerList& operator=(const LayerList&) = delete;  // Disabilita l'operatore di assegnazione

    QList<Layer> *layerList = new QList<Layer>;
};

#endif // LAYERLIST_H
