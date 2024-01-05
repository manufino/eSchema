#ifndef LAYERCOMBOBOX_H
#define LAYERCOMBOBOX_H

#include <QComboBox>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyleOptionComboBox>

#include "LayerItemDelegate.h"
#include "Layer.h"
#include "LayerList.h"


class LayerComboBox : public QComboBox {
    Q_OBJECT
public:
    LayerComboBox(QWidget* parent = nullptr);
    void addLayer(const QString& testo, const QColor& colore);
    void addLayerList(QList<Layer> *list);
    void setMaster(Layer *layer);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

signals:
    void layerSelectedChanged(int i);

public slots:
    void layerListIsChanged(QList<Layer> *layerList);

private slots:
    void currentIndexChanged(int index);

private:
    bool layerNameIsUnique(QString name);


private:
    QList<Layer> *layerList;
};

#endif // LAYERCOMBOBOX_H
