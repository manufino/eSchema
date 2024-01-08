#ifndef LAYERVISIBILITYBUTTON_H
#define LAYERVISIBILITYBUTTON_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>

#include "Layer.h"
#include "LayerList.h"

class LayerVisibilityButton : public QLabel
{
    Q_OBJECT

public:
    explicit LayerVisibilityButton(Layer * layer, QWidget *parent = nullptr);
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

#endif // LAYERVISIBILITYBUTTON_H
