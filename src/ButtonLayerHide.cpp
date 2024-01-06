#include "ButtonLayerHide.h"

ButtonLayerHide::ButtonLayerHide(QWidget *parent)
    : QLabel(parent)
{
    // Imposta l'immagine iniziale
    images.append(QPixmap(":/res/resources/remix/eye-line.png"));
    images.append(QPixmap(":/res/resources/remix/eye-off-line.png"));

    // setto una immagine iniziale,
    // altrimenti non si visualizza nulla se non..
    // viene specificato uno stato.
    setStatus(true);

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
}

void ButtonLayerHide::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        setStatus(!layerIsVisible);
    }

    QLabel::mousePressEvent(event);
}
