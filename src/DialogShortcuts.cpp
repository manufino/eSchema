#include "DialogShortcuts.h"
#include "ui_DialogShortcuts.h"

DialogShortcuts::DialogShortcuts(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogShortcuts)
{
    ui->setupUi(this);
}

DialogShortcuts::~DialogShortcuts()
{
    delete ui;
}
