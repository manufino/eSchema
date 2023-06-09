#include "statusbar.h"


StatusBar::StatusBar(QWidget *parent):QStatusBar(parent)
{
    lblPos->setText("X 0  Y 0");

    btnGrid->setCheckable(true);
    btnGrid->setChecked(true);
    QIcon icon(":/res/resources/grid.ico");
    btnGrid->setIcon(icon);
    btnGrid->setIconSize(QSize(24,24));
    btnGrid->setMinimumSize(QSize(24,24));
    btnGrid->setMaximumSize(QSize(24,24));
    btnGrid->setToolTip("Attiva o disattiva la griglia");


    btnSnapGrid->setCheckable(true);
    btnSnapGrid->setChecked(true);
    QIcon icon2(":/res/resources/magnet.png");
    btnSnapGrid->setIcon(icon2);
    btnSnapGrid->setIconSize(QSize(24,24));
    btnSnapGrid->setMinimumSize(QSize(24,24));
    btnSnapGrid->setMaximumSize(QSize(24,24));
    btnSnapGrid->setToolTip("Attiva o disattiva \nil snap sulla griglia");

    lblZoomLevel->setText("100%");

    this->addPermanentWidget(lblZoomLevel);
    this->addPermanentWidget(lblPos);
    this->addPermanentWidget(btnGrid);
    this->addPermanentWidget(btnSnapGrid);
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
