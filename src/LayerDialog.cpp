#include "LayerDialog.h"
#include "ui_LayerDialog.h"


LayerDialog::LayerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerDialog)
{
    ui->setupUi(this);

/*
    Layer l1("layer 1");
    Layer l3("layer 2", QColor("red"));
    Layer l4("lay5555555555555555er 3", QColor("blue"));
    Layer l5("afaafef", QColor("green"));
    Layer l6("*************** aef eae", QColor("cyan"));
    Layer l7("faeeaf3 aef", QColor("brown"));

    ui->listWidget->addLayer(&l1);
    ui->listWidget->addLayer(&l3);
    ui->listWidget->addLayer(&l4);
    ui->listWidget->addLayer(&l5);
    ui->listWidget->addLayer(&l6);
    ui->listWidget->addLayer(&l7);
*/
    /*
    LayerItemWidget *l1 = new LayerItemWidget(this);
    ui->listWidget->addItem()
*/
}

LayerDialog::~LayerDialog()
{
    delete ui;
}
