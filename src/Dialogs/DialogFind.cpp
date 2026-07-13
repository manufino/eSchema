/*
 * DialogFind.cpp
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

#include "DialogFind.h"
#include "ui_DialogFind.h"

DialogFind::DialogFind(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogFind)
{
    ui->setupUi(this);

    connect(ui->txtSearch, &QLineEdit::returnPressed, this, [this]() {
        emit findNext(ui->txtSearch->text());
    });
    connect(ui->btnNext, &QPushButton::clicked, this, [this]() {
        emit findNext(ui->txtSearch->text());
    });
    connect(ui->btnPrevious, &QPushButton::clicked, this, [this]() {
        emit findPrevious(ui->txtSearch->text());
    });
}

DialogFind::~DialogFind()
{
    delete ui;
}

void DialogFind::setStatusText(const QString &text)
{
    ui->lblStatus->setText(text);
}
