#ifndef BUTTONLAYERHIDE_H
#define BUTTONLAYERHIDE_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>

#include "Layer.h"
#include "LayerList.h"

class ButtonLayerHide : public QLabel
{
    Q_OBJECT

public:
    explicit ButtonLayerHide(Layer * layer, QWidget *parent = nullptr);
    void setStatus(bool status);
    bool getStatus() {return layerIsVisible;}

signals:
    void statusChanged(bool isVisible);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private:
    QList<QPixmap> images;
    bool layerIsVisible;
    Layer *layer;
};

#endif // BUTTONLAYERHIDE_H
