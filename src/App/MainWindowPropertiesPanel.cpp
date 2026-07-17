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
#include "PrimitivePad.h"
#include "PrimitivePcbTrack.h"
#include "PrimitiveComplexCurve.h"
#include "PrimitiveLine.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
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
    const QSignalBlocker blockArrowStart(ui->checkBoxArrowStart);
    const QSignalBlocker blockArrowEnd(ui->checkBoxArrowEnd);
    const QSignalBlocker blockArrowStyle(ui->comboArrowStyle);
    const QSignalBlocker blockArrowLength(ui->spinArrowLength);
    const QSignalBlocker blockArrowHalfWidth(ui->spinArrowHalfWidth);
    const QSignalBlocker blockCurveClosed(ui->checkBoxCurveClosed);
    const QSignalBlocker blockTrackWidth(ui->spinTrackWidth);
    const QSignalBlocker blockPadWidth(ui->spinPadWidth);
    const QSignalBlocker blockPadHeight(ui->spinPadHeight);
    const QSignalBlocker blockPadHole(ui->spinPadHole);
    const QSignalBlocker blockPadStyle(ui->cbPadStyle);
    const QSignalBlocker blockTextContent(ui->lineEditTextContent);
    const QSignalBlocker blockTextSizeX(ui->spinTextSizeX);
    const QSignalBlocker blockTextSizeY(ui->spinTextSizeY);
    const QSignalBlocker blockTextOrientation(ui->spinTextOrientation);
    const QSignalBlocker blockTextBold(ui->checkBoxTextBold);
    const QSignalBlocker blockTextItalic(ui->checkBoxTextItalic);
    const QSignalBlocker blockTextMirrored(ui->checkBoxTextMirrored);
    const QSignalBlocker blockLineLength(ui->spinLineLength);
    const QSignalBlocker blockShapeWidth(ui->spinShapeWidth);
    const QSignalBlocker blockShapeHeight(ui->spinShapeHeight);

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
        showRow(ui->labelArrowStart, ui->checkBoxArrowStart, false);
        showRow(ui->labelArrowEnd, ui->checkBoxArrowEnd, false);
        showRow(ui->labelArrowStyle, ui->comboArrowStyle, false);
        showRow(ui->labelArrowLength, ui->spinArrowLength, false);
        showRow(ui->labelArrowHalfWidth, ui->spinArrowHalfWidth, false);
        showRow(ui->labelCurveClosed, ui->checkBoxCurveClosed, false);
        showRow(ui->labelTrackWidth, ui->spinTrackWidth, false);
        showRow(ui->labelPadWidth, ui->spinPadWidth, false);
        showRow(ui->labelPadHeight, ui->spinPadHeight, false);
        showRow(ui->labelPadHole, ui->spinPadHole, false);
        showRow(ui->labelPadStyle, ui->cbPadStyle, false);
        showRow(ui->labelTextContent, ui->lineEditTextContent, false);
        showRow(ui->labelTextSizeX, ui->spinTextSizeX, false);
        showRow(ui->labelTextSizeY, ui->spinTextSizeY, false);
        showRow(ui->labelTextOrientation, ui->spinTextOrientation, false);
        showRow(ui->labelTextBold, ui->checkBoxTextBold, false);
        showRow(ui->labelTextItalic, ui->checkBoxTextItalic, false);
        showRow(ui->labelTextMirrored, ui->checkBoxTextMirrored, false);
        showRow(ui->labelLineLength, ui->spinLineLength, false);
        showRow(ui->labelShapeWidth, ui->spinShapeWidth, false);
        showRow(ui->labelShapeHeight, ui->spinShapeHeight, false);
        ui->lineEdit->clear();
        ui->lineEdit_2->clear();
        // The panel is otherwise completely blank - tell the user why,
        // instead of leaving a mysteriously empty dock. The trailing
        // spacer must stop expanding meanwhile, or it would keep half the
        // vertical space for itself and push the (also expanding) hint row
        // into the bottom half instead of letting it center in the dock.
        ui->labelNoSelection->setVisible(true);
        ui->verticalSpacerProp->changeSize(20, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        ui->gridLayoutProp->invalidate();
        return;
    }
    ui->labelNoSelection->setVisible(false);
    ui->verticalSpacerProp->changeSize(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    ui->gridLayoutProp->invalidate();

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
    showRow(ui->labelTextContent, ui->lineEditTextContent, isText);
    showRow(ui->labelTextSizeX, ui->spinTextSizeX, isText);
    showRow(ui->labelTextSizeY, ui->spinTextSizeY, isText);
    showRow(ui->labelTextOrientation, ui->spinTextOrientation, isText);
    showRow(ui->labelTextBold, ui->checkBoxTextBold, isText);
    showRow(ui->labelTextItalic, ui->checkBoxTextItalic, isText);
    showRow(ui->labelTextMirrored, ui->checkBoxTextMirrored, isText);
    if (isText) {
        auto *text = static_cast<PrimitiveText *>(primitive);
        ui->lineEditTextContent->setText(text->text());
        ui->fontComboBox->setCurrentFont(QFont(text->fontName()));
        ui->spinTextSizeX->setValue(text->sizeX());
        ui->spinTextSizeY->setValue(text->sizeY());
        ui->spinTextOrientation->setValue(text->orientationDeg());
        ui->checkBoxTextBold->setChecked(text->styleFlags() & PrimitiveText::Bold);
        ui->checkBoxTextItalic->setChecked(text->styleFlags() & PrimitiveText::Italic);
        ui->checkBoxTextMirrored->setChecked(text->styleFlags() & PrimitiveText::Mirrored);
    }

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

    // supportsArrows() alone correctly covers Line, Bezier, and an open
    // ComplexCurve all at once (it's already false for a closed one - arrows
    // are meaningless on a closed curve), no per-type switch needed here.
    const bool hasArrows = primitive->supportsArrows();
    showRow(ui->labelArrowStart, ui->checkBoxArrowStart, hasArrows);
    showRow(ui->labelArrowEnd, ui->checkBoxArrowEnd, hasArrows);
    showRow(ui->labelArrowStyle, ui->comboArrowStyle, hasArrows);
    showRow(ui->labelArrowLength, ui->spinArrowLength, hasArrows);
    showRow(ui->labelArrowHalfWidth, ui->spinArrowHalfWidth, hasArrows);
    if (hasArrows) {
        ui->checkBoxArrowStart->setChecked(primitive->arrowAtStart());
        ui->checkBoxArrowEnd->setChecked(primitive->arrowAtEnd());
        ui->comboArrowStyle->setCurrentArrowStyle(
                    (primitive->arrowStyleLimiter() ? 0x01 : 0)
                    | (primitive->arrowStyleEmpty() ? 0x02 : 0));
        ui->spinArrowLength->setValue(primitive->arrowLength());
        ui->spinArrowHalfWidth->setValue(primitive->arrowHalfWidth());
    }

    const bool isComplexCurve = primitive->getPrimitiveType() == GraphicsPrimitive::Spline;
    showRow(ui->labelCurveClosed, ui->checkBoxCurveClosed, isComplexCurve);
    if (isComplexCurve)
        ui->checkBoxCurveClosed->setChecked(static_cast<PrimitiveComplexCurve *>(primitive)->isClosed());

    const bool isPcbTrack = primitive->getPrimitiveType() == GraphicsPrimitive::PcbTrack;
    showRow(ui->labelTrackWidth, ui->spinTrackWidth, isPcbTrack);
    if (isPcbTrack)
        ui->spinTrackWidth->setValue(static_cast<PrimitivePcbTrack *>(primitive)->width());

    // Numeric geometry, limited to the primitives where a single number is
    // meaningful: a line's length, and a rectangle's/ellipse's bounding-box
    // size. Multi-point primitives (polygons, curves) have no such scalar.
    const bool isLine = primitive->getPrimitiveType() == GraphicsPrimitive::Line;
    showRow(ui->labelLineLength, ui->spinLineLength, isLine);
    if (isLine)
        ui->spinLineLength->setValue(static_cast<PrimitiveLine *>(primitive)->length());

    const bool isRectangle = primitive->getPrimitiveType() == GraphicsPrimitive::Rectangle;
    const bool isEllipse = primitive->getPrimitiveType() == GraphicsPrimitive::Ellipse;
    showRow(ui->labelShapeWidth, ui->spinShapeWidth, isRectangle || isEllipse);
    showRow(ui->labelShapeHeight, ui->spinShapeHeight, isRectangle || isEllipse);
    if (isRectangle) {
        auto *rectangle = static_cast<PrimitiveRectangle *>(primitive);
        ui->spinShapeWidth->setValue(rectangle->shapeWidth());
        ui->spinShapeHeight->setValue(rectangle->shapeHeight());
    } else if (isEllipse) {
        auto *ellipse = static_cast<PrimitiveEllipse *>(primitive);
        ui->spinShapeWidth->setValue(ellipse->shapeWidth());
        ui->spinShapeHeight->setValue(ellipse->shapeHeight());
    }

    const bool isPad = primitive->getPrimitiveType() == GraphicsPrimitive::Pad;
    showRow(ui->labelPadWidth, ui->spinPadWidth, isPad);
    showRow(ui->labelPadHeight, ui->spinPadHeight, isPad);
    showRow(ui->labelPadHole, ui->spinPadHole, isPad);
    showRow(ui->labelPadStyle, ui->cbPadStyle, isPad);
    if (isPad) {
        auto *pad = static_cast<PrimitivePad *>(primitive);
        ui->spinPadWidth->setValue(pad->outerWidth());
        ui->spinPadHeight->setValue(pad->outerHeight());
        ui->spinPadHole->setValue(pad->holeDiameter());
        ui->cbPadStyle->setCurrentIndex(static_cast<int>(pad->style()));
    }
}
