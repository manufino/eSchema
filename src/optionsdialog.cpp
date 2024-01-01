#include "OptionsDialog.h"
#include "ui_OptionsDialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
    this->setModal(true);

    connect(ui->btnOK, &QPushButton::clicked, this, &OptionsDialog::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &OptionsDialog::cancel);
    connect(ui->btnApply, &QPushButton::clicked, this, &OptionsDialog::apply);
    connect(ui->btnRestore, &QPushButton::clicked, this, &OptionsDialog::restore);

    loadSettings();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::loadSettings()
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
}

void OptionsDialog::saveSettings()
{
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

void OptionsDialog::accept()
{
    apply();
    close();
}

void OptionsDialog::cancel()
{
    close();
}

void OptionsDialog::apply()
{
    saveSettings();
}

void OptionsDialog::restore()
{
    SettingsManager::getInstance().restoreDefaultSettings();
    loadSettings();
}
