#include "statusbar.h"


StatusBar::StatusBar(QWidget *parent):QStatusBar(parent)
{

    lblPos->setText("X 200  Y 300");

    this->addWidget(lblPos, 0);
}

void StatusBar::GraphicsViewMousePos(QMouseEvent *event)
{
    int x = 0, y = 0;

    x = event->pos().x();
    y = event->pos().y();


    int gridSize = 10;
    x = round(x/gridSize)*gridSize;
    y = round(y/gridSize)*gridSize;

    QString pos = QString("X %1  Y %2  ").arg(x).arg(y);
    lblPos->setText(pos);
}
