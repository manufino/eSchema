#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QMouseEvent>

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);

public slots:
    void GraphicsViewMousePos(QMouseEvent *event);

private:
        QLabel *lblPos = new QLabel(this);
};

#endif // STATUSBAR_H
