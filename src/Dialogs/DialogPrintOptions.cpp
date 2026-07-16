/*
 * DialogPrintOptions.cpp
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

#include "DialogPrintOptions.h"
#include "ui_DialogPrintOptions.h"
#include "SettingsManager.h"
#include "LayerComboBox.h"

DialogPrintOptions::DialogPrintOptions(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogPrintOptions)
{
    ui->setupUi(this);
    loadSavedOptions();
}

DialogPrintOptions::~DialogPrintOptions()
{
    delete ui;
}

bool DialogPrintOptions::mirror() const
{
    return ui->chkMirror->isChecked();
}

bool DialogPrintOptions::blackWhite() const
{
    return ui->chkBlackWhite->isChecked();
}

bool DialogPrintOptions::landscape() const
{
    return ui->chkLandscape->isChecked();
}

void DialogPrintOptions::margins(double *top, double *bottom, double *left, double *right) const
{
    *top = ui->spinMarginTop->value();
    *bottom = ui->spinMarginBottom->value();
    *left = ui->spinMarginLeft->value();
    *right = ui->spinMarginRight->value();
}

bool DialogPrintOptions::realScale() const
{
    return ui->radioRealScale->isChecked();
}

double DialogPrintOptions::scalePercent() const
{
    return ui->spinScalePercent->value();
}

Layer *DialogPrintOptions::singleLayer() const
{
    return ui->chkOneLayer->isChecked() ? ui->cbPrintLayer->selectedLayer() : nullptr;
}

void DialogPrintOptions::accept()
{
    SettingsManager &settings = SettingsManager::getInstance();
    settings.saveSetting("print_mirror", ui->chkMirror->isChecked());
    settings.saveSetting("print_black_white", ui->chkBlackWhite->isChecked());
    settings.saveSetting("print_landscape", ui->chkLandscape->isChecked());
    settings.saveSetting("print_margin_top", ui->spinMarginTop->value());
    settings.saveSetting("print_margin_bottom", ui->spinMarginBottom->value());
    settings.saveSetting("print_margin_left", ui->spinMarginLeft->value());
    settings.saveSetting("print_margin_right", ui->spinMarginRight->value());
    settings.saveSetting("print_one_layer", ui->chkOneLayer->isChecked());
    settings.saveSetting("print_real_scale", ui->radioRealScale->isChecked());
    settings.saveSetting("print_scale_percent", ui->spinScalePercent->value());
    QDialog::accept();
}

void DialogPrintOptions::loadSavedOptions()
{
    const SettingsManager &settings = SettingsManager::getInstance();

    ui->chkMirror->setChecked(settings.loadSetting("print_mirror").toBool());
    ui->chkBlackWhite->setChecked(settings.loadSetting("print_black_white").toBool());
    ui->chkLandscape->setChecked(settings.loadSetting("print_landscape").toBool());

    const QVariant top = settings.loadSetting("print_margin_top");
    if (top.isValid())
        ui->spinMarginTop->setValue(top.toDouble());
    const QVariant bottom = settings.loadSetting("print_margin_bottom");
    if (bottom.isValid())
        ui->spinMarginBottom->setValue(bottom.toDouble());
    const QVariant left = settings.loadSetting("print_margin_left");
    if (left.isValid())
        ui->spinMarginLeft->setValue(left.toDouble());
    const QVariant right = settings.loadSetting("print_margin_right");
    if (right.isValid())
        ui->spinMarginRight->setValue(right.toDouble());

    // Not the specific layer itself (a Layer* isn't stable across sessions) -
    // just whether the checkbox starts ticked, defaulting to the combo's own
    // initial selection (the master layer, per LayerComboBox::setAutoMaster()).
    ui->chkOneLayer->setChecked(settings.loadSetting("print_one_layer").toBool());
    ui->cbPrintLayer->setEnabled(ui->chkOneLayer->isChecked());

    if (settings.loadSetting("print_real_scale").toBool())
        ui->radioRealScale->setChecked(true); // enables spinScalePercent via the .ui connection
    const QVariant percent = settings.loadSetting("print_scale_percent");
    if (percent.isValid())
        ui->spinScalePercent->setValue(percent.toDouble());
}
