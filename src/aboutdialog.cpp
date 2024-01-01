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

    ui->lblWebSite->setLink("eSchema su GitHub", QUrl("https://github.com/manufino/eSchema"));
    ui->lblRemixicon->setLink("Remixicon", QUrl("https://remixicon.com/"));
    ui->lblFidoCADJ->setLink("FidoCADJ", QUrl("https://darwinne.github.io/FidoCadJ/"));
    ui->lblDaFont->setLink("dafont.com", QUrl("https://www.dafont.com/"));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
