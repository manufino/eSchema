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
#include "PrimitiveLine.h"
#include "PrimitivePad.h"
#include "PrimitiveRectangle.h"
#include "PrimitiveEllipse.h"
#include "PrimitiveText.h"

#include <QInputDialog>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>

namespace {

enum PackageType { Dip = 0, Sip = 1, Quad = 2 };
enum MountStyle { ThroughHole = 0, Smd = 1, Schematic = 2 };

// Size of the pin-number/name labels, in drawing units (sizeY, sizeX).
constexpr int NumberSizeY = 8;
constexpr int NumberSizeX = 6;

// `horizontal`: an SMD pad is elongated toward the outside of the package -
// along x for the left/right pad rows, along y for the top/bottom ones.
PrimitivePad *makePad(const QPointF &center, int padSize, int holeSize, bool isPinOne,
                      bool smd, bool horizontal)
{
    auto *pad = new PrimitivePad();
    pad->setControlPoint(0, center);
    if (smd) {
        // SMD: rectangular, no hole, elongated 2:1 like a typical SOIC/QFP
        // land. Pin 1 is told apart by the marker dot, as on real parts.
        pad->setOuterSize(horizontal ? padSize * 2 : padSize,
                          horizontal ? padSize : padSize * 2);
        pad->setHoleDiameter(0);
        pad->setStyle(PrimitivePad::Rectangular);
    } else {
        pad->setOuterSize(padSize, padSize);
        pad->setHoleDiameter(qMin(holeSize, padSize - 1));
        // Pin 1 is the square pad - the universal footprint convention.
        pad->setStyle(isPinOne ? PrimitivePad::Rectangular : PrimitivePad::Round);
    }
    return pad;
}

PrimitiveText *makeLabel(const QString &text, const QPointF &topLeft)
{
    auto *label = new PrimitiveText();
    label->setText(text);
    label->setSize(NumberSizeY, NumberSizeX);
    label->setControlPoint(0, topLeft);
    return label;
}

PrimitiveLine *makePinLine(const QPointF &from, const QPointF &to)
{
    auto *line = new PrimitiveLine();
    line->setControlPoint(0, from);
    line->setControlPoint(1, to);
    return line;
}

PrimitiveEllipse *makePinOneMarker(const QPointF &topLeftCorner)
{
    auto *marker = new PrimitiveEllipse();
    marker->setControlPoint(0, topLeftCorner);
    marker->setControlPoint(1, topLeftCorner + QPointF(6, 6));
    return marker;
}

qreal textWidth(const QString &text)
{
    return text.size() * NumberSizeX;
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
    auto syncAndRefresh = [this](int) {
        syncTypeDependentFields();
        refreshPreview();
    };
    connect(ui->cboxType, &QComboBox::currentIndexChanged, this, syncAndRefresh);
    connect(ui->cboxStyle, &QComboBox::currentIndexChanged, this, syncAndRefresh);
    connect(ui->spinPins, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinPitch, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinRowSpacing, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinPadSize, &QSpinBox::valueChanged, this, refresh);
    connect(ui->spinHoleSize, &QSpinBox::valueChanged, this, refresh);
    connect(ui->chkPinNumbers, &QCheckBox::toggled, this, refresh);
    connect(ui->txtPinNames, &QPlainTextEdit::textChanged, this, refresh);

    // OK needs somewhere to save to and something to call the result.
    auto validate = [this]() {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                !ui->txtName->text().trimmed().isEmpty()
                && !ui->cboxLibrary->currentText().trimmed().isEmpty());
    };
    connect(ui->txtName, &QLineEdit::textChanged, this, validate);
    connect(ui->cboxLibrary, &QComboBox::editTextChanged, this, validate);

    // "+": create a brand-new library right here - the combo is editable
    // anyway (typing an unknown name creates it on save), but an explicit
    // button makes that discoverable.
    connect(ui->btnNewLibrary, &QToolButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("New library"),
                                                   tr("Library name:"),
                                                   QLineEdit::Normal, QString(), &ok);
        if (!ok || name.trimmed().isEmpty())
            return;
        ui->cboxLibrary->addItem(name.trimmed());
        ui->cboxLibrary->setCurrentIndex(ui->cboxLibrary->count() - 1);
    });

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
    const int type = ui->cboxType->currentIndex();
    const int style = ui->cboxStyle->currentIndex();
    switch (type) {
    case Dip:
        ui->spinPins->setMinimum(4);
        ui->spinPins->setSingleStep(2);
        break;
    case Sip:
        ui->spinPins->setMinimum(2);
        ui->spinPins->setSingleStep(1);
        break;
    case Quad:
        ui->spinPins->setMinimum(8);
        ui->spinPins->setSingleStep(4);
        break;
    }
    // Pads and holes only exist on footprints; the hole only on the
    // through-hole one. Row spacing doubles as the schematic body width
    // (and a quad schematic body is sized from its own pin span).
    ui->spinPadSize->setEnabled(style != Schematic);
    ui->spinHoleSize->setEnabled(style == ThroughHole);
    ui->spinRowSpacing->setEnabled((type != Sip && style != Schematic)
                                   || (style == Schematic && type != Quad));
    ui->lblRowSpacing->setText(style == Schematic ? tr("Body width (units)")
                                                  : tr("Row spacing (units)"));
}

QList<GraphicsPrimitive *> DialogSymbolWizard::buildPrimitives() const
{
    const int type = ui->cboxType->currentIndex();
    const int style = ui->cboxStyle->currentIndex();
    int pins = ui->spinPins->value();
    if (type == Dip)
        pins -= pins % 2;
    else if (type == Quad)
        pins -= pins % 4;
    const int pitch = ui->spinPitch->value();
    const int padSize = ui->spinPadSize->value();
    const int holeSize = ui->spinHoleSize->value();
    // Row spacing can't be smaller than what the pads themselves occupy
    // (SMD pads stick out twice as far).
    const int padReach = style == Smd ? padSize * 2 : padSize;
    const int rowSpacing = style == Schematic
            ? qMax(ui->spinRowSpacing->value(), 8 * NumberSizeX)
            : qMax(ui->spinRowSpacing->value(), padReach + 12);
    const bool numbers = ui->chkPinNumbers->isChecked();
    const bool smd = style == Smd;

    // Optional per-pin names, one per line - drawn next to the numbers, or
    // in their place when numbers are off.
    QStringList names = ui->txtPinNames->toPlainText().split(QLatin1Char('\n'));
    for (QString &name : names)
        name = name.trimmed();
    auto pinName = [&names](int pin) {
        return pin - 1 < names.size() ? names.at(pin - 1) : QString();
    };

    QList<GraphicsPrimitive *> primitives;

    if (style == Schematic) {
        // Schematic symbol: a body rectangle with plain line pins, numbers
        // outside above each pin, names just inside the body.
        const int pinLength = qMax(10, pitch);
        const qreal textMid = NumberSizeY / 2.0;

        if (type == Quad) {
            const int perSide = pins / 4;
            const qreal span = (perSide - 1) * pitch;
            const qreal side = span + 20; // 10 units of body beyond the end pins
            for (int i = 0; i < perSide; ++i) {
                const qreal along = 10 + i * pitch;
                const qreal alongBack = side - 10 - i * pitch; // CCW on right/top
                // Left side, pins 1..perSide top to bottom.
                primitives.append(makePinLine(QPointF(-pinLength, along), QPointF(0, along)));
                // Bottom, left to right.
                primitives.append(makePinLine(QPointF(along, side), QPointF(along, side + pinLength)));
                // Right, bottom to top.
                primitives.append(makePinLine(QPointF(side, alongBack), QPointF(side + pinLength, alongBack)));
                // Top, right to left.
                primitives.append(makePinLine(QPointF(alongBack, -pinLength), QPointF(alongBack, 0)));
                if (numbers) {
                    primitives.append(makeLabel(QString::number(i + 1),
                            QPointF(-pinLength, along - NumberSizeY - 1)));
                    primitives.append(makeLabel(QString::number(perSide + i + 1),
                            QPointF(along + 2, side + pinLength - NumberSizeY)));
                    const QString rightNumber = QString::number(2 * perSide + i + 1);
                    primitives.append(makeLabel(rightNumber,
                            QPointF(side + pinLength - textWidth(rightNumber),
                                    alongBack - NumberSizeY - 1)));
                    primitives.append(makeLabel(QString::number(3 * perSide + i + 1),
                            QPointF(alongBack + 2, -pinLength)));
                }
                const QString leftName = pinName(i + 1);
                if (!leftName.isEmpty())
                    primitives.append(makeLabel(leftName, QPointF(3, along - textMid)));
                const QString bottomName = pinName(perSide + i + 1);
                if (!bottomName.isEmpty())
                    primitives.append(makeLabel(bottomName,
                            QPointF(along - textWidth(bottomName) / 2.0, side - NumberSizeY - 3)));
                const QString rightName = pinName(2 * perSide + i + 1);
                if (!rightName.isEmpty())
                    primitives.append(makeLabel(rightName,
                            QPointF(side - 3 - textWidth(rightName), alongBack - textMid)));
                const QString topName = pinName(3 * perSide + i + 1);
                if (!topName.isEmpty())
                    primitives.append(makeLabel(topName,
                            QPointF(alongBack - textWidth(topName) / 2.0, 3)));
            }
            auto *body = new PrimitiveRectangle();
            body->setControlPoint(0, QPointF(0, 0));
            body->setControlPoint(1, QPointF(side, side));
            primitives.append(body);
            primitives.append(makePinOneMarker(QPointF(3, 3)));
        } else {
            // Dual row (pins on both sides, counterclockwise) or single row
            // (all pins on the left). Body width from the row-spacing field.
            const int rows = type == Dip ? pins / 2 : pins;
            const qreal width = rowSpacing;
            for (int i = 0; i < rows; ++i) {
                const qreal y = i * pitch;
                primitives.append(makePinLine(QPointF(-pinLength, y), QPointF(0, y)));
                if (numbers)
                    primitives.append(makeLabel(QString::number(i + 1),
                            QPointF(-pinLength, y - NumberSizeY - 1)));
                const QString leftName = pinName(i + 1);
                if (!leftName.isEmpty())
                    primitives.append(makeLabel(leftName, QPointF(3, y - textMid)));

                if (type == Dip) {
                    const int rightPin = pins - i; // CCW: back up the right side
                    primitives.append(makePinLine(QPointF(width, y),
                                                  QPointF(width + pinLength, y)));
                    if (numbers) {
                        const QString rightNumber = QString::number(rightPin);
                        primitives.append(makeLabel(rightNumber,
                                QPointF(width + pinLength - textWidth(rightNumber),
                                        y - NumberSizeY - 1)));
                    }
                    const QString rightName = pinName(rightPin);
                    if (!rightName.isEmpty())
                        primitives.append(makeLabel(rightName,
                                QPointF(width - 3 - textWidth(rightName), y - textMid)));
                }
            }
            auto *body = new PrimitiveRectangle();
            body->setControlPoint(0, QPointF(0, -pitch / 2.0));
            body->setControlPoint(1, QPointF(width, (rows - 1) * pitch + pitch / 2.0));
            primitives.append(body);
            primitives.append(makePinOneMarker(QPointF(3, -pitch / 2.0 + 3)));
        }
        return primitives;
    }

    if (type == Sip) {
        // A single left-to-right row; SMD pads elongate downward, away from
        // the body strip above.
        for (int i = 0; i < pins; ++i) {
            const QPointF center(i * pitch, 0);
            primitives.append(makePad(center, padSize, holeSize, i == 0, smd, false));
            qreal labelTop = padReach / 2.0 + 2;
            if (numbers) {
                primitives.append(makeLabel(QString::number(i + 1),
                        center + QPointF(-NumberSizeX / 2.0, labelTop)));
                labelTop += NumberSizeY + 2;
            }
            const QString name = pinName(i + 1);
            if (!name.isEmpty())
                primitives.append(makeLabel(name,
                        center + QPointF(-textWidth(name) / 2.0, labelTop)));
        }
        auto *body = new PrimitiveRectangle();
        body->setControlPoint(0, QPointF(-pitch / 2.0, -padReach / 2.0 - 4));
        body->setControlPoint(1, QPointF((pins - 1) * pitch + pitch / 2.0, padReach / 2.0 + 4));
        primitives.append(body);
    } else if (type == Dip) {
        // Counterclockwise numbering: 1..n/2 down the left column, n/2+1..n
        // back up the right one.
        const int half = pins / 2;
        const qreal bodyLeft = padReach / 2.0 + 2;
        const qreal bodyRight = rowSpacing - padReach / 2.0 - 2;
        for (int i = 0; i < half; ++i) {
            const QPointF leftCenter(0, i * pitch);
            const QPointF rightCenter(rowSpacing, i * pitch);
            primitives.append(makePad(leftCenter, padSize, holeSize, i == 0, smd, true));
            primitives.append(makePad(rightCenter, padSize, holeSize, false, smd, true));
            const qreal textTop = i * pitch - NumberSizeY / 2.0;
            const int rightPin = pins - i;
            // Left labels grow inward from the body edge: number first,
            // then the name (or the name alone with numbers off).
            qreal leftCursor = bodyLeft + 2;
            if (numbers) {
                primitives.append(makeLabel(QString::number(i + 1),
                                            QPointF(leftCursor, textTop)));
                leftCursor += textWidth(QString::number(i + 1)) + 4;
            }
            const QString leftName = pinName(i + 1);
            if (!leftName.isEmpty())
                primitives.append(makeLabel(leftName, QPointF(leftCursor, textTop)));
            // Right labels mirror it, right-aligned against the body edge.
            qreal rightCursor = bodyRight - 2;
            if (numbers) {
                const QString rightNumber = QString::number(rightPin);
                rightCursor -= textWidth(rightNumber);
                primitives.append(makeLabel(rightNumber, QPointF(rightCursor, textTop)));
                rightCursor -= 4;
            }
            const QString rightName = pinName(rightPin);
            if (!rightName.isEmpty())
                primitives.append(makeLabel(rightName,
                        QPointF(rightCursor - textWidth(rightName), textTop)));
        }
        auto *body = new PrimitiveRectangle();
        body->setControlPoint(0, QPointF(bodyLeft, -pitch / 2.0));
        body->setControlPoint(1, QPointF(bodyRight, (half - 1) * pitch + pitch / 2.0));
        primitives.append(body);
        primitives.append(makePinOneMarker(QPointF(bodyLeft + 3, -pitch / 2.0 + 3)));
    } else {
        // Quad: n/4 pads per side, counterclockwise from the left side's
        // top pad - left top-to-bottom, bottom left-to-right, right
        // bottom-to-top, top right-to-left.
        const int perSide = pins / 4;
        const qreal span = (perSide - 1) * pitch;
        const qreal low = -span / 2.0, high = span / 2.0;
        const qreal offset = rowSpacing / 2.0;
        const qreal bodyMargin = padReach / 2.0 + 2;
        const qreal bodyLeft = -offset + bodyMargin, bodyRight = offset - bodyMargin;
        const qreal bodyTop = -offset + bodyMargin, bodyBottom = offset - bodyMargin;
        for (int i = 0; i < perSide; ++i) {
            const qreal along = low + i * pitch;
            const qreal alongBack = high - i * pitch;
            primitives.append(makePad(QPointF(-offset, along), padSize, holeSize, i == 0, smd, true));
            primitives.append(makePad(QPointF(along, offset), padSize, holeSize, false, smd, false));
            primitives.append(makePad(QPointF(offset, alongBack), padSize, holeSize, false, smd, true));
            primitives.append(makePad(QPointF(alongBack, -offset), padSize, holeSize, false, smd, false));

            // Left/right sides: number first from the body edge, name after
            // it (or the name alone with numbers off) - same stacking as DIP.
            const qreal sideTextTop = along - NumberSizeY / 2.0;
            qreal leftCursor = bodyLeft + 2;
            if (numbers) {
                primitives.append(makeLabel(QString::number(i + 1),
                                            QPointF(leftCursor, sideTextTop)));
                leftCursor += textWidth(QString::number(i + 1)) + 4;
            }
            const QString leftName = pinName(i + 1);
            if (!leftName.isEmpty())
                primitives.append(makeLabel(leftName, QPointF(leftCursor, sideTextTop)));

            const int rightPin = 2 * perSide + i + 1;
            const qreal rightTextTop = alongBack - NumberSizeY / 2.0;
            qreal rightCursor = bodyRight - 2;
            if (numbers) {
                const QString rightNumber = QString::number(rightPin);
                rightCursor -= textWidth(rightNumber);
                primitives.append(makeLabel(rightNumber, QPointF(rightCursor, rightTextTop)));
                rightCursor -= 4;
            }
            const QString rightName = pinName(rightPin);
            if (!rightName.isEmpty())
                primitives.append(makeLabel(rightName,
                        QPointF(rightCursor - textWidth(rightName), rightTextTop)));

            // Bottom/top sides: number against the body edge, name stacked
            // one row further inside.
            const int bottomPin = perSide + i + 1;
            qreal bottomCursor = bodyBottom - NumberSizeY - 2;
            if (numbers) {
                primitives.append(makeLabel(QString::number(bottomPin),
                        QPointF(along - NumberSizeX, bottomCursor)));
                bottomCursor -= NumberSizeY + 2;
            }
            const QString bottomName = pinName(bottomPin);
            if (!bottomName.isEmpty())
                primitives.append(makeLabel(bottomName,
                        QPointF(along - textWidth(bottomName) / 2.0, bottomCursor)));

            const int topPin = 3 * perSide + i + 1;
            qreal topCursor = bodyTop + 2;
            if (numbers) {
                primitives.append(makeLabel(QString::number(topPin),
                        QPointF(alongBack - NumberSizeX, topCursor)));
                topCursor += NumberSizeY + 2;
            }
            const QString topName = pinName(topPin);
            if (!topName.isEmpty())
                primitives.append(makeLabel(topName,
                        QPointF(alongBack - textWidth(topName) / 2.0, topCursor)));
        }
        auto *body = new PrimitiveRectangle();
        body->setControlPoint(0, QPointF(bodyLeft, bodyTop));
        body->setControlPoint(1, QPointF(bodyRight, bodyBottom));
        primitives.append(body);
        primitives.append(makePinOneMarker(QPointF(bodyLeft + 3, bodyTop + 3)));
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
