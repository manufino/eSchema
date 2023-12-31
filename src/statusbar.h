#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include "settings_manager.h"

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);
    QPushButton *btnGrid = new QPushButton(this);
    QPushButton *btnSnapGrid = new QPushButton(this);

private:
    void LoadSettings();

public slots:
    void SceneMousePos(QPointF point);
    void ZoomLevel(unsigned int level);
    void SettingChanged();

signals:
    void ZoomChanged(unsigned int level);

private:
        QLabel *lblPos = new QLabel(this);
        QLabel *lblZoomLevel = new QLabel(this);
        int gridSize;
        double mm_step;

};

#endif // STATUSBAR_H
