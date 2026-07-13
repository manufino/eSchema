/*
 * DialogAttachImage.cpp
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

#include "DialogAttachImage.h"
#include "ui_DialogAttachImage.h"
#include "Sheet.h"

#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSignalBlocker>

DialogAttachImage::DialogAttachImage(const Sheet *sheet, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogAttachImage)
{
    ui->setupUi(this);
    setModal(true);

    connect(ui->btnBrowse, &QPushButton::clicked, this, &DialogAttachImage::browseForImage);
    connect(ui->btnRemoveImage, &QPushButton::clicked, this, &DialogAttachImage::removeImage);
    connect(ui->spinResolution, &QDoubleSpinBox::valueChanged, this, &DialogAttachImage::updateSizeFromResolution);
    connect(ui->spinWidthMm, &QDoubleSpinBox::valueChanged, this, &DialogAttachImage::updateHeightFromWidth);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    ui->btnRemoveImage->setEnabled(sheet->hasBackgroundImage());

    if (sheet->hasBackgroundImage()) {
        m_image = sheet->backgroundImage();
        m_mimeSubtype = sheet->backgroundImageMimeSubtype();
        m_base64Data = sheet->backgroundImageBase64();
        ui->txtImagePath->setText(tr("(currently attached image)"));
        ui->spinResolution->setValue(sheet->backgroundImageResolution());
        ui->spinX->setValue(sheet->backgroundImageCorner().x());
        ui->spinY->setValue(sheet->backgroundImageCorner().y());
        updateSizeFromResolution();
    } else {
        updateHeightLabel();
    }
}

DialogAttachImage::~DialogAttachImage()
{
    delete ui;
}

double DialogAttachImage::resolution() const
{
    return ui->spinResolution->value();
}

QPointF DialogAttachImage::corner() const
{
    return QPointF(ui->spinX->value(), ui->spinY->value());
}

void DialogAttachImage::browseForImage()
{
    const QString path = QFileDialog::getOpenFileName(
                this, tr("Tracing image"), QString(),
                tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!path.isEmpty())
        loadImageFile(path);
}

void DialogAttachImage::loadImageFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Tracing image"), tr("Unable to read the file:\n%1").arg(path));
        return;
    }
    const QByteArray data = file.readAll();

    QImage image;
    if (!image.loadFromData(data)) {
        QMessageBox::warning(this, tr("Tracing image"), tr("Unrecognized image file:\n%1").arg(path));
        return;
    }

    // FIDOSPECS.md 5.12's mime subtype convention, not a full "image/..."
    // string - ".jpg" is the common file extension but "jpeg" is the real
    // subtype, matching PrimitiveImage's own image-loading convention.
    QString mimeSubtype = QFileInfo(path).suffix().toLower();
    if (mimeSubtype == QStringLiteral("jpg"))
        mimeSubtype = QStringLiteral("jpeg");

    m_image = image;
    m_mimeSubtype = mimeSubtype;
    m_base64Data = QString::fromLatin1(data.toBase64());
    m_imageRemoved = false;
    ui->txtImagePath->setText(path);
    ui->btnRemoveImage->setEnabled(true);
    updateSizeFromResolution();
}

void DialogAttachImage::updateSizeFromResolution()
{
    if (m_image.isNull() || ui->spinResolution->value() <= 0)
        return;
    const QSignalBlocker blocker(ui->spinWidthMm);
    ui->spinWidthMm->setValue(m_image.width() / ui->spinResolution->value() * 25.4);
    updateHeightLabel();
}

void DialogAttachImage::updateHeightFromWidth()
{
    if (m_image.isNull() || ui->spinWidthMm->value() <= 0 || m_image.width() <= 0)
        return;
    const QSignalBlocker blocker(ui->spinResolution);
    ui->spinResolution->setValue(m_image.width() / ui->spinWidthMm->value() * 25.4);
    updateHeightLabel();
}

void DialogAttachImage::updateHeightLabel()
{
    if (m_image.isNull() || ui->spinResolution->value() <= 0) {
        ui->lblHeightMmValue->setText(QStringLiteral("-"));
        return;
    }
    const double heightMm = m_image.height() / ui->spinResolution->value() * 25.4;
    ui->lblHeightMmValue->setText(QString::number(heightMm, 'f', 1));
}

void DialogAttachImage::removeImage()
{
    m_imageRemoved = true;
    accept();
}
