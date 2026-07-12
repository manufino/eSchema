/*
 * DialogLayerList.cpp
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "DialogLayerList.h"
#include "ui_DialogLayerList.h"
#include "LayerIcons.h"

DialogLayerList::DialogLayerList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogLayerList)
{
    ui->setupUi(this);

    ui->btnAddNewLayer->setProperty("class", "layoutLayerListButton");
    ui->btnDeleteLayer->setProperty("class", "layoutLayerListButton");
    ui->btnLayerLevelDown->setProperty("class", "layoutLayerListButton");
    ui->btnLayerLevelUp->setProperty("class", "layoutLayerListButton");
    ui->btnSetAllVisible->setProperty("class", "layoutLayerListButton");
    ui->btnSetAllHidden->setProperty("class", "layoutLayerListButton");
    ui->btnSetAllLocked->setProperty("class", "layoutLayerListButton");
    ui->btnSetAllUnlocked->setProperty("class", "layoutLayerListButton");

    // Runtime-rendered (no lock icon assets exist in resources.qrc), unlike
    // the eye buttons' .ui-declared icons - see LayerIcons.h.
    ui->btnSetAllLocked->setIcon(QIcon(LayerIcons::renderLockIcon(true)));
    ui->btnSetAllLocked->setIconSize(QSize(25, 25));
    ui->btnSetAllUnlocked->setIcon(QIcon(LayerIcons::renderLockIcon(false)));
    ui->btnSetAllUnlocked->setIconSize(QSize(25, 25));

    connect(ui->btnAddNewLayer, &QPushButton::clicked,
            this, &DialogLayerList::addNewLayer);
    connect(ui->btnDeleteLayer, &QPushButton::clicked,
            this, &DialogLayerList::deleteCurrent);
    connect(ui->btnLayerLevelUp, &QPushButton::clicked,
            this, &DialogLayerList::levelUp);
    connect(ui->btnLayerLevelDown, &QPushButton::clicked,
            this, &DialogLayerList::levelDown);
    connect(ui->btnSetAllVisible, &QPushButton::clicked,
            this, &DialogLayerList::setAllVisible);
    connect(ui->btnSetAllHidden, &QPushButton::clicked,
            this, &DialogLayerList::setAllHidden);
    connect(ui->btnSetAllLocked, &QPushButton::clicked,
            this, &DialogLayerList::setAllLocked);
    connect(ui->btnSetAllUnlocked, &QPushButton::clicked,
            this, &DialogLayerList::setAllUnlocked);
}

DialogLayerList::~DialogLayerList()
{
    delete ui;
}

void DialogLayerList::levelUp()
{
    if(layerIsSelected())
    {
        Layer *l = ui->listWidget->getSelectedLayer();
        LayerList::getInstance().moveUp(l);
        ui->listWidget->updateList();
        ui->listWidget->setSelectedLayer(l);
    }
}

void DialogLayerList::levelDown()
{
    if(layerIsSelected())
    {
        Layer *l = ui->listWidget->getSelectedLayer();
        LayerList::getInstance().moveDown(l);
        ui->listWidget->updateList();
        ui->listWidget->setSelectedLayer(l);
    }
}

void DialogLayerList::setAllVisible()
{
    LayerList::getInstance().setAllVisibleOrHidden(true);
    ui->listWidget->updateList();
}

void DialogLayerList::setAllHidden()
{
    LayerList::getInstance().setAllVisibleOrHidden(false);
    ui->listWidget->updateList();
}

void DialogLayerList::setAllLocked()
{
    LayerList::getInstance().setAllLockedOrUnlocked(true);
    ui->listWidget->updateList();
}

void DialogLayerList::setAllUnlocked()
{
    LayerList::getInstance().setAllLockedOrUnlocked(false);
    ui->listWidget->updateList();
}

void DialogLayerList::deleteCurrent()
{
    if(layerIsSelected())
    {
        // the master layer can't be deleted
        if(ui->listWidget->getSelectedLayer()->isMaster())
            return;

        LayerList::getInstance().removeLayer(ui->listWidget->getSelectedLayer());
        ui->listWidget->updateList();
    }
}

void DialogLayerList::addNewLayer()
{
    QString ln = QString("%1").arg(ui->listWidget->count());

    Layer *layer = new Layer("Nuovo layer " + ln);
    LayerList::getInstance().addLayer(layer);
    ui->listWidget->updateList();
}

bool DialogLayerList::layerIsSelected()
{
    if(ui->listWidget->selectedItems().size() == 0)
        return false;
    else return true;
}

QColor DialogLayerList::randomColor()
{
    int red = QRandomGenerator::global()->bounded(256);
    int green = QRandomGenerator::global()->bounded(256);
    int blue = QRandomGenerator::global()->bounded(256);

    return QColor(red, green, blue);
}
