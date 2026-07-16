/*
 * DialogSymbolWizard.cpp
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

#include "DialogSymbolWizard.h"
#include "ui_DialogSymbolWizard.h"
#include "LibraryManager.h"
#include "PrimitivePad.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "PrimitiveText.h"

#include <QPainter>
#include <QPixmap>
#include <QPushButton>

namespace {

enum PackageType { Dip = 0, Sip = 1, Quad = 2 };

// Size of the pin-number labels, in drawing units (sizeY, sizeX).
constexpr int NumberSizeY = 8;
constexpr int NumberSizeX = 6;

PrimitivePad *makePad(const QPointF &center, int padSize, int holeSize, bool isPinOne)
{
    auto *pad = new PrimitivePad();
    pad->setControlPoint(0, center);
    pad->setOuterSize(padSize, padSize);
    pad->setHoleDiameter(qMin(holeSize, padSize - 1));
    // Pin 1 is the square pad - the universal footprint convention.
    pad->setStyle(isPinOne ? PrimitivePad::Rectangular : PrimitivePad::Round);
    return pad;
}

PrimitiveText *makeNumber(int pin, const QPointF &topLeft)
{
    auto *text = new PrimitiveText();
    text->setText(QString::number(pin));
    text->setSize(NumberSizeY, NumberSizeX);
    text->setControlPoint(0, topLeft);
    return text;
}

} // namespace

DialogSymbolWizard::DialogSymbolWizard(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSymbolWizard)
{
    ui->setupUi(this);

    // Save targets: the existing user libraries; typing a new name creates
    // that library on save (standard libraries are read-only).
    ui->cboxLibrary->addItems(LibraryManager::getInstance().userLibraryDisplayNames());
    if (ui->cboxLibrary->count() == 0)
        ui->cboxLibrary->setEditText(tr("My symbols"));

    auto refresh = [this]() { refreshPreview(); };
    connect(ui->cboxType, &QComboBox::currentIndexChanged, this, [this](int) {
        syncTypeDependentFields();
        refreshPreview();
    });
    connect(ui->spinPins, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinPitch, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinRowSpacing, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinPadSize, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinHoleSize, &QSpinBox::valueChanged, this, refresh);
    connect(ui->chkPinNumbers, &QCheckBox::toggled, this, refresh);

    // OK needs somewhere to save to and something to call the result.
    auto validate = [this]() {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !ui->txtName->text().trimmed().isEmpty()
                && !ui->cboxLibrary->currentText().trimmed().isEmpty());
    };
    connect(ui->txtName, &QLineEdit::textChanged, this, validate);
    connect(ui->cboxLibrary, &QComboBox::editTextChanged, this, validate);

    syncTypeDependentFields();
    refreshPreview();
    validate();
}

DialogSymbolWizard::~DialogSymbolWizard()
{
    delete ui;
}

void DialogSymbolWizard::syncTypeDependentFields()
{
    // DIP needs an even pin count, a quad a multiple of four - the spin box
    // steps/minimums enforce it as the user scrolls; out-of-step typed
    // values are rounded in buildPrimitives().
    switch (ui->cboxType->currentIndex()) {
    case Dip:
        ui->spinPins->setMinimum(4);
        ui->spinPins->setSingleStep(2);
        ui->spinRowSpacing->setEnabled(true);
        break;
    case Sip:
        ui->spinPins->setMinimum(2);
        ui->spinPins->setSingleStep(1);
        ui->spinRowSpacing->setEnabled(false);
        break;
    case Quad:
        ui->spinPins->setMinimum(8);
        ui->spinPins->setSingleStep(4);
        ui->spinRowSpacing->setEnabled(true);
        break;
    }
}

QList<GraphicsPrimitive *> DialogSymbolWizard::buildPrimitives() const
{
    const int type = ui->cboxType->currentIndex();
    int pins = ui->spinPins->value();
    if (type == Dip)
        pins -= pins % 2;
    else if (type == Quad)
        pins -= pins % 4;
    const int pitch = ui->spinPitch->value();
    const int padSize = ui->spinPadSize->value();
    const int holeSize = ui->spinHoleSize->value();
    // Row spacing can't be smaller than what the pads themselves occupy.
    const int rowSpacing = qMax(ui->spinRowSpacing->value(), padSize + 12);
    const bool numbers = ui->chkPinNumbers->isChecked();

    QList<GraphicsPrimitive *> primitives;

    if (type == Sip) {
        // A single left-to-right row, body strip above the numbers.
        for (int i = 0; i < pins; ++i) {
            const QPointF center(i * pitch, 0);
            primitives.append(makePad(center, padSize, holeSize, i == 0));
            if (numbers)
                primitives.append(makeNumber(i + 1, center + QPointF(-NumberSizeX / 2.0,
                                                                     padSize / 2.0 + 2)));
        }
        auto *body = new PrimitiveRectangle();
        body->setControlPoint(0, QPointF(-pitch / 2.0, -padSize / 2.0 - 4));
        body->setControlPoint(1, QPointF((pins - 1) * pitch + pitch / 2.0, padSize / 2.0 + 4));
        primitives.append(body);
    } else if (type == Dip) {
        // Counterclockwise numbering: 1..n/2 down the left column, n/2+1..n
        // back up the right one.
        const int half = pins / 2;
        const qreal bodyLeft = padSize / 2.0 + 2;
        const qreal bodyRight = rowSpacing - padSize / 2.0 - 2;
        for (int i = 0; i < half; ++i) {
            const QPointF leftCenter(0, i * pitch);
            const QPointF rightCenter(rowSpacing, i * pitch);
            primitives.append(makePad(leftCenter, padSize, holeSize, i == 0));
            primitives.append(makePad(rightCenter, padSize, holeSize, false));
            if (numbers) {
                primitives.append(makeNumber(i + 1,
                        QPointF(bodyLeft + 2, i * pitch - NumberSizeY / 2.0)));
                const QString rightNumber = QString::number(pins - i);
                primitives.append(makeNumber(pins - i,
                        QPointF(bodyRight - 2 - rightNumber.size() * NumberSizeX,
                                i * pitch - NumberSizeY / 2.0)));
            }
        }
        auto *body = new PrimitiveRectangle();
        body->setControlPoint(0, QPointF(bodyLeft, -pitch / 2.0));
        body->setControlPoint(1, QPointF(bodyRight, (half - 1) * pitch + pitch / 2.0));
        primitives.append(body);
        // Pin-1 marker: a small circle inside the body's top-left corner.
        auto *marker = new PrimitiveEllipse();
        marker->setControlPoint(0, QPointF(bodyLeft + 3, -pitch / 2.0 + 3));
        marker->setControlPoint(1, QPointF(bodyLeft + 9, -pitch / 2.0 + 9));
        primitives.append(marker);
    } else {
        // Quad: n/4 pads per side, counterclockwise from the left side's
        // top pad - left top-to-bottom, bottom left-to-right, right
        // bottom-to-top, top right-to-left.
        const int perSide = pins / 4;
        const qreal span = (perSide - 1) * pitch;
        const qreal low = -span / 2.0, high = span / 2.0;
        const qreal offset = rowSpacing / 2.0;
        const qreal bodyMargin = padSize / 2.0 + 2;
        const qreal bodyLeft = -offset + bodyMargin, bodyRight = offset - bodyMargin;
        const qreal bodyTop = -offset + bodyMargin, bodyBottom = offset - bodyMargin;
        for (int i = 0; i < perSide; ++i) {
            const qreal along = low + i * pitch;
            const QPointF left(-offset, along);
            const QPointF bottom(along, offset);
            const QPointF right(offset, high - i * pitch);
            const QPointF top(high - i * pitch, -offset);
            primitives.append(makePad(left, padSize, holeSize, i == 0));
            primitives.append(makePad(bottom, padSize, holeSize, false));
            primitives.append(makePad(right, padSize, holeSize, false));
            primitives.append(makePad(top, padSize, holeSize, false));
            if (numbers) {
                primitives.append(makeNumber(i + 1,
                        QPointF(bodyLeft + 2, along - NumberSizeY / 2.0)));
                primitives.append(makeNumber(perSide + i + 1,
                        QPointF(along - NumberSizeX, bodyBottom - NumberSizeY - 2)));
                const QString rightNumber = QString::number(2 * perSide + i + 1);
                primitives.append(makeNumber(2 * perSide + i + 1,
                        QPointF(bodyRight - 2 - rightNumber.size() * NumberSizeX,
                                high - i * pitch - NumberSizeY / 2.0)));
                primitives.append(makeNumber(3 * perSide + i + 1,
                        QPointF(high - i * pitch - NumberSizeX, bodyTop + 2)));
            }
        }
        auto *body = new PrimitiveRectangle();
        body->setControlPoint(0, QPointF(bodyLeft, bodyTop));
        body->setControlPoint(1, QPointF(bodyRight, bodyBottom));
        primitives.append(body);
        auto *marker = new PrimitiveEllipse();
        marker->setControlPoint(0, QPointF(bodyLeft + 3, bodyTop + 3));
        marker->setControlPoint(1, QPointF(bodyLeft + 9, bodyTop + 9));
        primitives.append(marker);
    }

    return primitives;
}

void DialogSymbolWizard::refreshPreview()
{
    const QList<GraphicsPrimitive *> primitives = buildPrimitives();

    QRectF bounds;
    for (GraphicsPrimitive *primitive : primitives)
        bounds = bounds.united(primitive->boundingRect());

    const int side = qMin(ui->lblPreview->width(), ui->lblPreview->height()) - 12;
    QPixmap pixmap(qMax(64, side), qMax(64, side));
    // Always a plain white sheet, like MacroPreviewWidget: symbols are drawn
    // in colors chosen against a white canvas, whatever the app theme.
    pixmap.fill(Qt::white);
    if (bounds.isValid() && bounds.width() > 0 && bounds.height() > 0) {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal margin = pixmap.width() * 0.08;
        const qreal scale = qMin((pixmap.width() - 2 * margin) / bounds.width(),
                                 (pixmap.height() - 2 * margin) / bounds.height());
        painter.translate(pixmap.width() / 2.0, pixmap.height() / 2.0);
        painter.scale(scale, scale);
        painter.translate(-bounds.center());
        for (GraphicsPrimitive *primitive : primitives)
            primitive->paint(&painter, nullptr, nullptr);
    }
    ui->lblPreview->setPixmap(pixmap);

    qDeleteAll(primitives);
}

QString DialogSymbolWizard::macroName() const
{
    return ui->txtName->text().trimmed();
}

QString DialogSymbolWizard::category() const
{
    const QString category = ui->txtCategory->text().trimmed();
    return category.isEmpty() ? tr("Wizard") : category;
}

QString DialogSymbolWizard::libraryDisplayName() const
{
    return ui->cboxLibrary->currentText().trimmed();
}

QString DialogSymbolWizard::libraryFilename() const
{
    const QString displayName = libraryDisplayName();
    const QString existing = LibraryManager::getInstance().userLibraryFilename(displayName);
    if (!existing.isEmpty())
        return existing;
    // A new library: derive a plain-ASCII filename prefix from the display
    // name, same spirit as DialogCreateMacro's own new-library handling.
    QString filename;
    for (const QChar &character : displayName) {
        if (character.isLetterOrNumber())
            filename.append(character.toLower());
        else if (character.isSpace() || character == QLatin1Char('_')
                 || character == QLatin1Char('-'))
            filename.append(QLatin1Char('_'));
    }
    return filename.isEmpty() ? QStringLiteral("wizard") : filename;
}
