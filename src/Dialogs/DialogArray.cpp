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
#include "SheetView.h"
#include "Sheet.h"
#include <QMouseEvent>

DialogArray::DialogArray(SheetView *view, QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogArray), m_view(view)
{
    ui->setupUi(this);
    connect(ui->comboLayout, &QComboBox::currentIndexChanged,
            this, [this]() { syncModeVisibility(); });
    connect(ui->btnPickCenter, &QPushButton::clicked,
            this, [this]() { startCenterPick(); });
    ui->btnPickCenter->setEnabled(m_view != nullptr);
    syncModeVisibility();
}

DialogArray::~DialogArray()
{
    if (m_picking)
        endCenterPick();
    delete ui;
}

// Hiding a modal dialog releases its input block, so the canvas becomes
// clickable while this dialog waits invisibly for the pick.
void DialogArray::startCenterPick()
{
    if (!m_view || m_picking)
        return;
    m_picking = true;
    hide();
    m_view->viewport()->installEventFilter(this);
    m_view->installEventFilter(this); // key events (Escape) land on the view
    m_view->viewport()->setCursor(Qt::CrossCursor);
    m_view->setFocus();
}

void DialogArray::endCenterPick()
{
    if (!m_picking)
        return;
    m_picking = false;
    m_view->viewport()->removeEventFilter(this);
    m_view->removeEventFilter(this);
    m_view->viewport()->unsetCursor();
    if (auto *sheet = qobject_cast<Sheet *>(m_view->scene()))
        sheet->clearSnapIndicator();
    show();
    raise();
    activateWindow();
}

bool DialogArray::eventFilter(QObject *watched, QEvent *event)
{
    if (m_picking) {
        auto *sheet = qobject_cast<Sheet *>(m_view->scene());
        if (watched == m_view->viewport() && event->type() == QEvent::MouseButtonPress) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                const QPointF scenePos = m_view->mapToScene(mouse->position().toPoint());
                // Same snapping as a drawing click - object snap included,
                // so the center can land exactly on an existing point.
                const QPointF snapped = sheet ? sheet->snapPosition(scenePos) : scenePos;
                setSuggestedCenter(snapped);
                endCenterPick();
                return true; // the click must not also select/draw anything
            }
            if (mouse->button() == Qt::RightButton) {
                endCenterPick(); // cancelled - keep the typed coordinates
                return true;
            }
        } else if (watched == m_view->viewport() && event->type() == QEvent::MouseMove) {
            // Live object-snap highlight while hunting for the point.
            if (sheet)
                sheet->snapPosition(m_view->mapToScene(
                        static_cast<QMouseEvent *>(event)->position().toPoint()));
        } else if (event->type() == QEvent::KeyPress
                   && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            endCenterPick();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
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
