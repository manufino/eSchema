/*
 * DialogOptions.cpp
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "DialogOptions.h"
#include "ui_DialogOptions.h"
#include "ThemeManager.h"

#include <QFileDialog>

namespace {
// Matches cboxLanguage's item order 1:1 (see DialogOptions.ui) - index into
// this to get/set the "language" setting's value from/to the combo box.
const QStringList LanguageCodes = {
    "it", "en", "de", "fr", "es", "ru", "zh", "pl", "ja", "pt", "ar", "hi"
};

// Matches cboxStyle's item order 1:1 (Chiaro/Scuro/Sistema/Stylesheet, see
// DialogOptions.ui) - index into this to get/set the "gui_style" setting's
// value from/to the combo box. Also consumed by ThemeManager::apply().
const QStringList GuiStyleCodes = {
    "light", "dark", "system", "custom"
};
}

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
    connect(ui->cboxStyle, &QComboBox::currentIndexChanged, this, &DialogOptions::updateStylesheetPathEnabled);
    connect(ui->btnOpenStylesheetPath, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, tr("Seleziona stylesheet"),
                                                            ui->txtStylesheetPath->text(),
                                                            tr("Stylesheet (*.qss)"));
        if (!path.isEmpty())
            ui->txtStylesheetPath->setText(path);
    });

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
    ui->cboxGridType->setCurrentIndex(val.toInt());

    val = SettingsManager::getInstance().loadSetting("library_directory");
    ui->txtLibPath->setText(val.toString());

    val = SettingsManager::getInstance().loadSetting("gui_style");
    const int styleIndex = GuiStyleCodes.indexOf(val.toString());
    ui->cboxStyle->setCurrentIndex(styleIndex >= 0 ? styleIndex : 0);

    val = SettingsManager::getInstance().loadSetting("stylesheet_path");
    ui->txtStylesheetPath->setText(val.toString());

    updateStylesheetPathEnabled();

    val = SettingsManager::getInstance().loadSetting("snap_enabled");
    ui->chkSnapEnabled->setChecked(val.toBool());

    val = SettingsManager::getInstance().loadSetting("snap_step");
    ui->spinSnapStep->setValue(val.toInt());

    val = SettingsManager::getInstance().loadSetting("macro_icon_size");
    ui->spinMacroIconSize->setValue(val.toInt() > 0 ? val.toInt() : 32);

    val = SettingsManager::getInstance().loadSetting("line_width");
    ui->spinLineWidth->setValue(val.toDouble() > 0 ? val.toDouble() : 0.5);

    val = SettingsManager::getInstance().loadSetting("selection_tolerance");
    ui->spinSelectionTolerance->setValue(val.toDouble() > 0 ? val.toDouble() : 3.0);

    val = SettingsManager::getInstance().loadSetting("connection_diameter");
    ui->spinConnectionDiameter->setValue(val.toDouble() > 0 ? val.toDouble() : 2.0);

    val = SettingsManager::getInstance().loadSetting("language");
    const int languageIndex = LanguageCodes.indexOf(val.toString());
    ui->cboxLanguage->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    m_initialLanguageIndex = ui->cboxLanguage->currentIndex();
}

void DialogOptions::saveSettings()
{
    SettingsManager::getInstance().saveSetting("grid_type", ui->cboxGridType->currentIndex());
    SettingsManager::getInstance().saveSetting("grid_step", ui->spinGridStep->value());
    SettingsManager::getInstance().saveSetting("grid_line_width", ui->doubleSpinGridLineWidth->value());
    SettingsManager::getInstance().saveSetting("grid_line_mark_width", ui->doubleSpinGridLineMarkWidth->value());
    SettingsManager::getInstance().saveSetting("grid_line_color", ui->lblGridLineNormalColor->getColor().name());
    SettingsManager::getInstance().saveSetting("grid_line_mark_color", ui->lblGridLineMarkColor->getColor().name());
    SettingsManager::getInstance().saveSetting("grid_dot_color", ui->lblGridColor->getColor().name());
    SettingsManager::getInstance().saveSetting("background_color", ui->lblBackColor->getColor().name());
    SettingsManager::getInstance().saveSetting("grid_step_mark", ui->spinGridLineMarkStep->value());
    SettingsManager::getInstance().saveSetting("mm_step", ui->doubleSpinStep_mm->value());
    SettingsManager::getInstance().saveSetting("library_directory", ui->txtLibPath->text());
    SettingsManager::getInstance().saveSetting("gui_style", GuiStyleCodes.at(ui->cboxStyle->currentIndex()));
    SettingsManager::getInstance().saveSetting("stylesheet_path", ui->txtStylesheetPath->text());
    SettingsManager::getInstance().saveSetting("snap_enabled", ui->chkSnapEnabled->isChecked());
    SettingsManager::getInstance().saveSetting("snap_step", ui->spinSnapStep->value());
    SettingsManager::getInstance().saveSetting("macro_icon_size", ui->spinMacroIconSize->value());
    SettingsManager::getInstance().saveSetting("line_width", ui->spinLineWidth->value());
    SettingsManager::getInstance().saveSetting("selection_tolerance", ui->spinSelectionTolerance->value());
    SettingsManager::getInstance().saveSetting("connection_diameter", ui->spinConnectionDiameter->value());
    SettingsManager::getInstance().saveSetting("language", LanguageCodes.at(ui->cboxLanguage->currentIndex()));
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
    ThemeManager::apply();

    // The UI isn't retranslated live - the change is saved and takes effect
    // on the next launch, so tell the user rather than leaving them thinking
    // the combo box selection did nothing.
    if (ui->cboxLanguage->currentIndex() != m_initialLanguageIndex) {
        m_initialLanguageIndex = ui->cboxLanguage->currentIndex();
        QMessageBox::information(this, tr("Lingua"),
                                  tr("La nuova lingua sarà attiva al prossimo riavvio di eSchema."));
    }
}

void DialogOptions::updateStylesheetPathEnabled()
{
    const bool isCustom = ui->cboxStyle->currentIndex() == GuiStyleCodes.indexOf("custom");
    ui->txtStylesheetPath->setEnabled(isCustom);
    ui->btnOpenStylesheetPath->setEnabled(isCustom);
}

void DialogOptions::restore()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Attenzione !"),
                                  tr("Tutti i settaggi verranno sovrascritti.\n\n"
                                  "Procedere con il ripristino dei valori ?\n"),
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        SettingsManager::getInstance().restoreDefaultSettings();
        loadSettings();
    }
}
