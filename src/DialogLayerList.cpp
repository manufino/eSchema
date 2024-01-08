#include "DialogLayerList.h"
#include "ui_DialogLayerList.h"

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

void DialogLayerList::deleteCurrent()
{
    if(layerIsSelected())
    {
        // il layer master non si cancella
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
