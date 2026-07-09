/*
 * DialogAbout.cpp
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

#include "DialogAbout.h"
#include "ui_DialogAbout.h"

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
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

DialogAbout::~DialogAbout()
{
    delete ui;
}
