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

    lblZoomLevel->setText(" 7%");

    this->addPermanentWidget(lblZoomLevel);
    this->addPermanentWidget(lblPos);
    this->addPermanentWidget(btnGrid);
    this->addPermanentWidget(btnSnapGrid);
}

void StatusBar::SceneMousePos(QPointF point)
{
    int x = static_cast<int>(point.x());
    int y = static_cast<int>(point.y());

    int gridSize = 10;
    int halfGridSize = gridSize / 2;

    // Arrotonda alla griglia in base alla distanza dalla griglia
    x = (x + halfGridSize) / gridSize * gridSize;
    y = (y + halfGridSize) / gridSize * gridSize;

    QString pos = QString("X %1  Y %2").arg(x).arg(y);
    lblPos->setText(pos);
}


void StatusBar::ZoomLevel(unsigned int level)
{
    QString pos = QString(" %1%").arg(level);
    lblZoomLevel->setText(pos);
}

