#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include "SettingsManager.h"

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);
    QPushButton *btnGrid = new QPushButton(this);
    QPushButton *btnSnapGrid = new QPushButton(this);

private:
    void loadSettings();

public slots:
    void sceneMousePos(QPointF point);
    void zoomLevel(unsigned int level);
    void settingChanged();

signals:
    void zoomChanged(unsigned int level);

private:
        QLabel *lblPos = new QLabel(this);
        QLabel *lblZoomLevel = new QLabel(this);
        int gridSize;
        double mm_step;

};

#endif // STATUSBAR_H
