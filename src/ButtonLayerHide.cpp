#include "ButtonLayerHide.h"

ButtonLayerHide::ButtonLayerHide(Layer * layer, QWidget *parent)
    : QLabel(parent)
{
    this->layer = layer;

    images.append(QPixmap(":/res/resources/remix/eye-line.png"));
    images.append(QPixmap(":/res/resources/remix/eye-off-line.png"));
    images.append(QPixmap(":/res/resources/remix/eye-fill.png"));

    if(layer->isMaster())
        setPixmap(images[2]);
    else
        setStatus(layer->isVisible());

    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setText("");
}

void ButtonLayerHide::setStatus(bool status)
{
    if(status)
        setPixmap(images[0]);
    else
        setPixmap(images[1]);

    layerIsVisible = status;
    LayerList::getInstance().setVisible(layer, layerIsVisible);
}

void ButtonLayerHide::mousePressEvent(QMouseEvent *event)
{
    if(layer->isMaster())
        return;

    if (event->button() == Qt::LeftButton)
        setStatus(!layerIsVisible);

    QLabel::mousePressEvent(event);
}

void ButtonLayerHide::mouseMoveEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    if(layer->isMaster())
        setCursor(Qt::ArrowCursor);
    else setCursor(Qt::PointingHandCursor);
}
