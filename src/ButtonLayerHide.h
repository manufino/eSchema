#ifndef BUTTONLAYERHIDE_H
#define BUTTONLAYERHIDE_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>

class ButtonLayerHide : public QLabel
{
    Q_OBJECT

public:
    explicit ButtonLayerHide(QWidget *parent = nullptr);
    void setStatus(bool status);
    bool getStatus() {return layerIsVisible;}

signals:
    void statusChanged(bool isVisible);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QList<QPixmap> images;
    bool layerIsVisible;
};

#endif // BUTTONLAYERHIDE_H
