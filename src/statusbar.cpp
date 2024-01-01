#include "StatusBar.h"


StatusBar::StatusBar(QWidget *parent):QStatusBar(parent)
{
    lblPos->setText("X 0  Y 0 (X 0.0mm Y 0.0mm)");

    lblPos->setMinimumWidth(350);

    btnGrid->setCheckable(true);
    btnGrid->setChecked(true);
    QIcon icon(":/res/resources/remix/grid-line.png");
    btnGrid->setIcon(icon);
    btnGrid->setIconSize(QSize(24,24));
    btnGrid->setMinimumSize(QSize(24,24));
    btnGrid->setMaximumSize(QSize(24,24));
    btnGrid->setToolTip("Attiva o disattiva la griglia");

    btnSnapGrid->setCheckable(true);
    btnSnapGrid->setChecked(true);
    QIcon icon2(":/res/resources/remix/grid-fill.png");
    btnSnapGrid->setIcon(icon2);
    btnSnapGrid->setIconSize(QSize(24,24));
    btnSnapGrid->setMinimumSize(QSize(24,24));
    btnSnapGrid->setMaximumSize(QSize(24,24));
    btnSnapGrid->setToolTip("Attiva o disattiva \nil snap sulla griglia");

    lblZoomLevel->setText("Zoom 7%");

    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &StatusBar::SettingChanged);

    LoadSettings();

    this->addPermanentWidget(lblZoomLevel);
    this->addPermanentWidget(lblPos);
    this->addPermanentWidget(btnGrid);
    this->addPermanentWidget(btnSnapGrid);
}

void StatusBar::LoadSettings()
{
    QVariant val = SettingsManager::getInstance().loadSetting("grid_step");
    gridSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("mm_step");
    mm_step = val.toDouble();
}

void StatusBar::SceneMousePos(QPointF point)
{
    int x = static_cast<int>(point.x());
    int y = static_cast<int>(point.y());

    int halfGridSize = gridSize / 2;

    // Arrotonda alla griglia in base alla distanza dalla griglia
    x = (x + halfGridSize) / gridSize * gridSize;
    y = (y + halfGridSize) / gridSize * gridSize;

    double xmm = mm_step * (x/10);
    double ymm = mm_step * (y/10);

    QString pos = QString("X %1 Y %2 (X %3mm Y %4mm)").arg(x).arg(y).arg(xmm).arg(ymm);
    lblPos->setText(pos);
}


void StatusBar::ZoomLevel(unsigned int level)
{
    QString pos = QString("Zoom %1%").arg(level);
    lblZoomLevel->setText(pos);
}

void StatusBar::SettingChanged()
{
    LoadSettings();
    update();
}

