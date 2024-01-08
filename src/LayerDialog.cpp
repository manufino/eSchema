#include "LayerDialog.h"
#include "ui_LayerDialog.h"

LayerDialog::LayerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerDialog)
{
    ui->setupUi(this);

    ui->btnAddNewLayer->setProperty("class", "layoutLayerListButton");
    ui->btnDeleteLayer->setProperty("class", "layoutLayerListButton");
    ui->btnLayerLevelDown->setProperty("class", "layoutLayerListButton");
    ui->btnLayerLevelUp->setProperty("class", "layoutLayerListButton");
    ui->btnSetAllVisible->setProperty("class", "layoutLayerListButton");
    ui->btnSetAllHidden->setProperty("class", "layoutLayerListButton");

    connect(ui->btnAddNewLayer, &QPushButton::clicked,
            this, &LayerDialog::addNewLayer);
    connect(ui->btnDeleteLayer, &QPushButton::clicked,
            this, &LayerDialog::deleteCurrent);
    connect(ui->btnLayerLevelUp, &QPushButton::clicked,
            this, &LayerDialog::levelUp);
    connect(ui->btnLayerLevelDown, &QPushButton::clicked,
            this, &LayerDialog::levelDown);
    connect(ui->btnSetAllVisible, &QPushButton::clicked,
            this, &LayerDialog::setAllVisible);
    connect(ui->btnSetAllHidden, &QPushButton::clicked,
            this, &LayerDialog::setAllHidden);
}

LayerDialog::~LayerDialog()
{
    delete ui;
}

void LayerDialog::levelUp()
{
    if(layerIsSelected())
    {
        Layer *l = ui->listWidget->getSelectedLayer();
        LayerList::getInstance().moveUp(l);
        ui->listWidget->updateList();
        ui->listWidget->setSelectedLayer(l);
    }
}

void LayerDialog::levelDown()
{
    if(layerIsSelected())
    {
        Layer *l = ui->listWidget->getSelectedLayer();
        LayerList::getInstance().moveDown(l);
        ui->listWidget->updateList();
        ui->listWidget->setSelectedLayer(l);
    }
}

void LayerDialog::setAllVisible()
{
    LayerList::getInstance().setAllVisibleOrHidden(true);
    ui->listWidget->updateList();
}

void LayerDialog::setAllHidden()
{
    LayerList::getInstance().setAllVisibleOrHidden(false);
    ui->listWidget->updateList();
}

void LayerDialog::deleteCurrent()
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

void LayerDialog::addNewLayer()
{
    QString ln = QString("%1").arg(ui->listWidget->count());

    Layer *layer = new Layer("Nuovo layer " + ln, Utils::randomColor());
    LayerList::getInstance().addLayer(layer);
    ui->listWidget->updateList();
}

bool LayerDialog::layerIsSelected()
{
    if(ui->listWidget->selectedItems().size() == 0)
        return false;
    else return true;
}
