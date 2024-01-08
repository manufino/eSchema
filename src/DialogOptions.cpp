#include "DialogOptions.h"
#include "ui_DialogOptions.h"

DialogOptions::DialogOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogOptions)
{
    ui->setupUi(this);
    this->setModal(true);

    connect(ui->btnOK, &QPushButton::clicked, this, &DialogOptions::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &DialogOptions::cancel);
    connect(ui->btnApply, &QPushButton::clicked, this, &DialogOptions::apply);
    connect(ui->btnRestore, &QPushButton::clicked, this, &DialogOptions::restore);

    loadSettings();
}

DialogOptions::~DialogOptions()
{
    delete ui;
}

void DialogOptions::loadSettings()
{
    QVariant val = SettingsManager::getInstance().loadSetting("grid_step");
    ui->spinGridStep->setValue(val.toInt());

    val = SettingsManager::getInstance().loadSetting("grid_line_width");
    ui->doubleSpinGridLineWidth->setValue(val.toDouble());

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_width");
    ui->doubleSpinGridLineMarkWidth->setValue(val.toDouble());

    val = SettingsManager::getInstance().loadSetting("grid_line_color");
    ui->lblGridLineNormalColor->setColor(QColor(val.toString()));

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_color");
    ui->lblGridLineMarkColor->setColor(QColor(val.toString()));

    val = SettingsManager::getInstance().loadSetting("grid_dot_color");
    ui->lblGridColor->setColor(QColor(val.toString()));

    val = SettingsManager::getInstance().loadSetting("background_color");
    ui->lblBackColor->setColor(QColor(val.toString()));

    val = SettingsManager::getInstance().loadSetting("grid_step_mark");
    ui->spinGridLineMarkStep->setValue(val.toInt());

    val = SettingsManager::getInstance().loadSetting("mm_step");
    ui->doubleSpinStep_mm->setValue(val.toDouble());

    val = SettingsManager::getInstance().loadSetting("grid_type");
    ui->cboxGridType->setCurrentText(val.toString());
}

void DialogOptions::saveSettings()
{
    SettingsManager::getInstance().saveSetting("grid_type", ui->cboxGridType->currentText());
    SettingsManager::getInstance().saveSetting("grid_step", ui->spinGridStep->value());
    SettingsManager::getInstance().saveSetting("grid_line_width", ui->doubleSpinGridLineWidth->value());
    SettingsManager::getInstance().saveSetting("grid_line_mark_width", ui->doubleSpinGridLineMarkWidth->value());
    SettingsManager::getInstance().saveSetting("grid_line_color", ui->lblGridLineNormalColor->getColor().name());
    SettingsManager::getInstance().saveSetting("grid_line_mark_color", ui->lblGridLineMarkColor->getColor().name());
    SettingsManager::getInstance().saveSetting("grid_dot_color", ui->lblGridColor->getColor().name());
    SettingsManager::getInstance().saveSetting("background_color", ui->lblBackColor->getColor().name());
    SettingsManager::getInstance().saveSetting("grid_step_mark", ui->spinGridLineMarkStep->value());
    SettingsManager::getInstance().saveSetting("mm_step", ui->doubleSpinStep_mm->value());
}

void DialogOptions::accept()
{
    apply();
    close();
}

void DialogOptions::cancel()
{
    close();
}

void DialogOptions::apply()
{
    saveSettings();
}

void DialogOptions::restore()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Attenzione !",
                                  "Tutti i settaggi verranno sovrascritti.\n\n"
                                  "Procedere con il ripristino dei valori ?\n",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        SettingsManager::getInstance().restoreDefaultSettings();
        loadSettings();
    }
}
