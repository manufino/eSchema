/*
 * DialogHatch.cpp
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

#include "DialogHatch.h"
#include "ui_DialogHatch.h"
#include "SettingsManager.h"

DialogHatch::DialogHatch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogHatch)
{
    ui->setupUi(this);

    // Last-used values, with the classic 45 degrees / 5 units as the
    // first-time defaults.
    const QVariant angle = SettingsManager::getInstance().loadSetting("hatch_angle");
    ui->spinAngle->setValue(angle.isValid() ? angle.toDouble() : 45.0);
    const QVariant spacing = SettingsManager::getInstance().loadSetting("hatch_spacing");
    ui->spinSpacing->setValue(spacing.isValid() && spacing.toDouble() > 0 ? spacing.toDouble() : 5.0);
    ui->chkCross->setChecked(SettingsManager::getInstance().loadSetting("hatch_cross").toBool());
}

DialogHatch::~DialogHatch()
{
    delete ui;
}

double DialogHatch::angleDeg() const
{
    return ui->spinAngle->value();
}

double DialogHatch::spacing() const
{
    return ui->spinSpacing->value();
}

bool DialogHatch::crossHatch() const
{
    return ui->chkCross->isChecked();
}

void DialogHatch::accept()
{
    SettingsManager::getInstance().saveSetting("hatch_angle", angleDeg());
    SettingsManager::getInstance().saveSetting("hatch_spacing", spacing());
    SettingsManager::getInstance().saveSetting("hatch_cross", crossHatch());
    QDialog::accept();
}
