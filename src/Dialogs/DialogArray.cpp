/*
 * DialogArray.cpp
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

#include "DialogArray.h"
#include "ui_DialogArray.h"

DialogArray::DialogArray(QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogArray)
{
    ui->setupUi(this);
    connect(ui->comboLayout, &QComboBox::currentIndexChanged,
            this, [this]() { syncModeVisibility(); });
    // The pick itself is the caller's job - see the class doc comment.
    connect(ui->btnPickCenter, &QPushButton::clicked,
            this, [this]() { done(PickRequested); });
    syncModeVisibility();
}

DialogArray::~DialogArray()
{
    delete ui;
}

void DialogArray::syncModeVisibility()
{
    const bool circular = mode() == Mode::Circular;
    ui->groupGrid->setVisible(!circular);
    ui->groupCircular->setVisible(circular);
    ui->labelHint->setVisible(!circular);
    adjustSize();
}

DialogArray::Mode DialogArray::mode() const
{
    return ui->comboLayout->currentIndex() == 1 ? Mode::Circular : Mode::Grid;
}

int DialogArray::columns() const
{
    return ui->spinColumns->value();
}

int DialogArray::rows() const
{
    return ui->spinRows->value();
}

qreal DialogArray::spacingX() const
{
    return ui->spinSpacingX->value();
}

qreal DialogArray::spacingY() const
{
    return ui->spinSpacingY->value();
}

int DialogArray::copies() const
{
    return ui->spinCopies->value();
}

qreal DialogArray::totalAngle() const
{
    return ui->spinTotalAngle->value();
}

QPointF DialogArray::center() const
{
    return QPointF(ui->spinCenterX->value(), ui->spinCenterY->value());
}

bool DialogArray::rotateCopies() const
{
    return ui->checkRotateCopies->isChecked();
}

void DialogArray::setSuggestedCenter(const QPointF &center)
{
    ui->spinCenterX->setValue(center.x());
    ui->spinCenterY->setValue(center.y());
}
