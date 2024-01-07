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
