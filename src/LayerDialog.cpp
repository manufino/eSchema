#include "LayerDialog.h"
#include "ui_LayerDialog.h"

#include "LayerListView.h"

LayerDialog::LayerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerDialog)
{
    ui->setupUi(this);

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

}

void LayerDialog::levelDown()
{

}

void LayerDialog::setAllVisible()
{

}

void LayerDialog::setAllHidden()
{

}

void LayerDialog::deleteCurrent()
{

}

void LayerDialog::addNewLayer()
{

}
