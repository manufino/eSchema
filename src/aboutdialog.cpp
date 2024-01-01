#include "AboutDialog.h"
#include "ui_AboutDialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    this->setModal(true);

    ui->lblSoftwareVersion->setText(APP_VERSION);
    ui->lblQtVersion->setText(QT_VERSION_STR);
    ui->lblCompileDate->setText(BUILDDATE);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
