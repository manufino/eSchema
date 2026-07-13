/*
 * DialogExport.cpp
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

#include "DialogExport.h"
#include "ui_DialogExport.h"
#include "SettingsManager.h"

#include <QtMath>

DialogExport::DialogExport(const QSizeF &drawingSize, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogExport)
    , m_drawingSize(drawingSize)
{
    ui->setupUi(this);

    // userData carries the GraphicExporter format token, so display labels
    // can change (or be translated) without touching any logic.
    ui->cbFormat->addItem(QStringLiteral("PNG (bitmap)"), QStringLiteral("png"));
    ui->cbFormat->addItem(QStringLiteral("JPG (bitmap)"), QStringLiteral("jpg"));
    ui->cbFormat->addItem(QStringLiteral("SVG (vettoriale)"), QStringLiteral("svg"));
    ui->cbFormat->addItem(QStringLiteral("PDF (vettoriale)"), QStringLiteral("pdf"));
    ui->cbFormat->addItem(QStringLiteral("EPS (vettoriale)"), QStringLiteral("eps"));
    ui->cbFormat->addItem(QStringLiteral("DXF (AutoCAD)"), QStringLiteral("dxf"));

    loadSavedOptions();

    connect(ui->cbFormat, &QComboBox::currentIndexChanged,
            this, &DialogExport::updateWidgetStates);
    connect(ui->rbResolution, &QRadioButton::toggled,
            this, &DialogExport::updateWidgetStates);
    connect(ui->spinResolution, &QDoubleSpinBox::valueChanged,
            this, &DialogExport::updateResultLabel);
    connect(ui->spinSizeX, &QSpinBox::valueChanged,
            this, &DialogExport::updateResultLabel);
    connect(ui->spinSizeY, &QSpinBox::valueChanged,
            this, &DialogExport::updateResultLabel);

    updateWidgetStates();
}

DialogExport::~DialogExport()
{
    delete ui;
}

QString DialogExport::selectedFormat() const
{
    return ui->cbFormat->currentData().toString();
}

GraphicExporter::Options DialogExport::options() const
{
    GraphicExporter::Options options;
    options.format = selectedFormat();
    options.resolutionBased = ui->rbResolution->isChecked();
    options.resolution = ui->spinResolution->value();
    options.totX = ui->spinSizeX->value();
    options.totY = ui->spinSizeY->value();
    options.antialias = ui->chkAntialias->isChecked();
    options.blackWhite = ui->chkBlackWhite->isChecked();
    options.splitLayers = ui->chkSplitLayers->isEnabled()
            && ui->chkSplitLayers->isChecked();
    return options;
}

QString DialogExport::fileFilter() const
{
    const QString format = selectedFormat();
    if (format == QLatin1String("png"))
        return tr("File PNG (*.png)");
    if (format == QLatin1String("jpg"))
        return tr("File JPG (*.jpg *.jpeg)");
    if (format == QLatin1String("svg"))
        return tr("File SVG (*.svg)");
    if (format == QLatin1String("pdf"))
        return tr("File PDF (*.pdf)");
    if (format == QLatin1String("eps"))
        return tr("File EPS (*.eps)");
    return tr("File DXF (*.dxf)");
}

QString DialogExport::defaultSuffix() const
{
    return selectedFormat();
}

void DialogExport::accept()
{
    SettingsManager &settings = SettingsManager::getInstance();
    settings.saveSetting("export_format", selectedFormat());
    settings.saveSetting("export_resolution_based", ui->rbResolution->isChecked());
    settings.saveSetting("export_resolution", ui->spinResolution->value());
    settings.saveSetting("export_size_x", ui->spinSizeX->value());
    settings.saveSetting("export_size_y", ui->spinSizeY->value());
    settings.saveSetting("export_antialias", ui->chkAntialias->isChecked());
    settings.saveSetting("export_black_white", ui->chkBlackWhite->isChecked());
    settings.saveSetting("export_split_layers", ui->chkSplitLayers->isChecked());
    QDialog::accept();
}

void DialogExport::loadSavedOptions()
{
    const SettingsManager &settings = SettingsManager::getInstance();

    const int formatIndex = ui->cbFormat->findData(
                settings.loadSetting("export_format").toString());
    if (formatIndex >= 0)
        ui->cbFormat->setCurrentIndex(formatIndex);

    const QVariant resolutionBased = settings.loadSetting("export_resolution_based");
    if (resolutionBased.isValid()) {
        ui->rbResolution->setChecked(resolutionBased.toBool());
        ui->rbSize->setChecked(!resolutionBased.toBool());
    }
    const QVariant resolution = settings.loadSetting("export_resolution");
    if (resolution.isValid() && resolution.toDouble() > 0)
        ui->spinResolution->setValue(resolution.toDouble());
    const QVariant sizeX = settings.loadSetting("export_size_x");
    if (sizeX.isValid() && sizeX.toInt() > 0)
        ui->spinSizeX->setValue(sizeX.toInt());
    const QVariant sizeY = settings.loadSetting("export_size_y");
    if (sizeY.isValid() && sizeY.toInt() > 0)
        ui->spinSizeY->setValue(sizeY.toInt());
    const QVariant antialias = settings.loadSetting("export_antialias");
    if (antialias.isValid())
        ui->chkAntialias->setChecked(antialias.toBool());
    ui->chkBlackWhite->setChecked(settings.loadSetting("export_black_white").toBool());
    ui->chkSplitLayers->setChecked(settings.loadSetting("export_split_layers").toBool());
}

void DialogExport::updateWidgetStates()
{
    const QString format = selectedFormat();
    const bool bitmap = format == QLatin1String("png")
            || format == QLatin1String("jpg");
    const bool dxf = format == QLatin1String("dxf");

    ui->groupSizing->setEnabled(!dxf);
    ui->chkAntialias->setEnabled(bitmap);
    ui->chkSplitLayers->setEnabled(!dxf);

    const bool resolutionMode = ui->rbResolution->isChecked();
    ui->spinResolution->setEnabled(!dxf && resolutionMode);
    ui->spinSizeX->setEnabled(!dxf && !resolutionMode);
    ui->spinSizeY->setEnabled(!dxf && !resolutionMode);

    updateResultLabel();
}

void DialogExport::updateResultLabel()
{
    if (selectedFormat() == QLatin1String("dxf")) {
        ui->lblResult->clear();
        return;
    }
    QSizeF size;
    if (ui->rbResolution->isChecked())
        size = m_drawingSize * ui->spinResolution->value();
    else
        size = QSizeF(ui->spinSizeX->value(), ui->spinSizeY->value());
    ui->lblResult->setText(tr("Immagine risultante: %1 x %2 pixel")
                                   .arg(qCeil(size.width()))
                                   .arg(qCeil(size.height())));
}
