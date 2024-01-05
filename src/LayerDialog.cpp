#include "LayerDialog.h"
#include "ui_LayerDialog.h"

LayerDialog::LayerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerDialog)
{
    ui->setupUi(this);
}

LayerDialog::~LayerDialog()
{
    delete ui;
}
