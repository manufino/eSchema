/*
 * MainWindowPropertiesPanel.cpp
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

// MainWindow's Properties dock: syncing its fields to the current selection.
// Editing back out to the selected primitives happens in setConnections()
// (MainWindow.cpp), since each field's edit signal is wired there alongside
// every other widget connection. See MainWindow.cpp's top-of-file comment
// for why this lives in its own translation unit instead of inside
// MainWindow.cpp itself.

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "PrimitiveText.h"
#include "PrimitiveImage.h"
#include <QSignalBlocker>
#include <QFont>

GraphicsPrimitive *MainWindow::firstSelectedPrimitive() const
{
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->isSelected())
            return primitive;
    }
    return nullptr;
}

QList<GraphicsPrimitive *> MainWindow::selectedPrimitivesInOrder() const
{
    QList<GraphicsPrimitive *> result;
    for (GraphicsPrimitive *primitive : sheetScene->primitives()) {
        if (primitive->isSelected())
            result.append(primitive);
    }
    return result;
}

void MainWindow::updatePropertiesPanel()
{
    GraphicsPrimitive *primitive = firstSelectedPrimitive();

    const QSignalBlocker blockName(ui->lineEdit);
    const QSignalBlocker blockValue(ui->lineEdit_2);
    const QSignalBlocker blockLayer(ui->cbPropLayer);
    const QSignalBlocker blockFill(ui->checkBox);
    const QSignalBlocker blockStyle(ui->cbPropLineStyle);
    const QSignalBlocker blockFont(ui->fontComboBox);
    const QSignalBlocker blockOpacity(ui->spinOpacity);
    const QSignalBlocker blockKeepAspectRatio(ui->checkBoxKeepAspectRatio);
    const QSignalBlocker blockGrayscale(ui->checkBoxGrayscale);

    auto showRow = [](QWidget *label, QWidget *field, bool visible) {
        label->setVisible(visible);
        field->setVisible(visible);
    };

    if (!primitive) {
        // Nothing selected - the whole panel goes blank, matching the
        // request that it not double as a "next primitive" defaults editor.
        showRow(ui->label_2, ui->lineEdit, false);
        showRow(ui->label_3, ui->lineEdit_2, false);
        showRow(ui->label_4, ui->cbPropLayer, false);
        showRow(ui->label_6, ui->checkBox, false);
        showRow(ui->label_7, ui->cbPropLineStyle, false);
        showRow(ui->label_8, ui->fontComboBox, false);
        showRow(ui->label_9, ui->spinOpacity, false);
        showRow(ui->label_10, ui->checkBoxKeepAspectRatio, false);
        showRow(ui->label_11, ui->checkBoxGrayscale, false);
        ui->lineEdit->clear();
        ui->lineEdit_2->clear();
        return;
    }

    // Nome/Valore apply to every primitive type (FIDOSPECS.md 6.1-6.3: every
    // primitive can carry a name/value TY label, one way or another).
    showRow(ui->label_2, ui->lineEdit, true);
    showRow(ui->label_3, ui->lineEdit_2, true);
    ui->lineEdit->setText(primitive->name());
    ui->lineEdit_2->setText(primitive->value());

    // FCD macros ("MC") have no layer token at all (FIDOSPECS.md 5.10 - they
    // are always logically on layer 0) - showing an editable layer field
    // that silently has no effect on the saved file would be misleading.
    const bool hasLayer = primitive->getPrimitiveType() != GraphicsPrimitive::PartLib;
    showRow(ui->label_4, ui->cbPropLayer, hasLayer);
    if (hasLayer && primitive->layer())
        ui->cbPropLayer->setMaster(primitive->layer());

    bool hasFill = false;
    bool hasLineStyle = false;
    switch (primitive->getPrimitiveType()) {
    case GraphicsPrimitive::Rectangle:
    case GraphicsPrimitive::Ellipse:
    case GraphicsPrimitive::Polyline:
    case GraphicsPrimitive::Spline:
        hasFill = true;
        hasLineStyle = true;
        break;
    case GraphicsPrimitive::Line:
    case GraphicsPrimitive::Bezier:
        hasLineStyle = true;
        break;
    default:
        break;
    }
    showRow(ui->label_6, ui->checkBox, hasFill);
    if (hasFill)
        ui->checkBox->setChecked(primitive->isFilled());
    showRow(ui->label_7, ui->cbPropLineStyle, hasLineStyle);
    if (hasLineStyle)
        ui->cbPropLineStyle->setCurrentPenStyle(primitive->lineStyle());

    const bool isText = primitive->getPrimitiveType() == GraphicsPrimitive::Text;
    showRow(ui->label_8, ui->fontComboBox, isText);
    if (isText)
        ui->fontComboBox->setCurrentFont(QFont(static_cast<PrimitiveText *>(primitive)->fontName()));

    const bool isImage = primitive->getPrimitiveType() == GraphicsPrimitive::Image;
    showRow(ui->label_9, ui->spinOpacity, isImage);
    showRow(ui->label_10, ui->checkBoxKeepAspectRatio, isImage);
    showRow(ui->label_11, ui->checkBoxGrayscale, isImage);
    if (isImage) {
        auto *image = static_cast<PrimitiveImage *>(primitive);
        ui->spinOpacity->setValue(qRound(image->opacity() * 100.0));
        ui->checkBoxKeepAspectRatio->setChecked(image->keepAspectRatio());
        ui->checkBoxGrayscale->setChecked(image->isGrayscale());
    }
}
