#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);
    QPushButton *btnGrid = new QPushButton(this);
    QPushButton *btnSnapGrid = new QPushButton(this);

public slots:
    void GraphicsViewMousePos(QMouseEvent *event);


private:
        QLabel *lblPos = new QLabel(this);
        QLabel *lblZoomLevel = new QLabel(this);

};

#endif // STATUSBAR_H
