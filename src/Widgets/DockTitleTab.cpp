/*
 * DockTitleTab.cpp
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

#include "DockTitleTab.h"
#include <QDockWidget>
#include <QMouseEvent>
#include <QPainter>

DockTitleTab::DockTitleTab(QDockWidget *dock)
    : QWidget(dock), m_dock(dock)
{
    setMouseTracking(true); // hover feedback on the X without buttons down
    m_dock->installEventFilter(this);
}

QSize DockTitleTab::sizeHint() const
{
    return QSize(120, 22);
}

QRect DockTitleTab::closeRect() const
{
    const int size = 16;
    return QRect(width() - size - 4, (height() - size) / 2, size, size);
}

void DockTitleTab::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Tab-like body from the live palette, so every theme (and live theme
    // switches) are followed for free.
    const QColor window = palette().color(QPalette::Window);
    const QColor text = palette().color(QPalette::WindowText);
    const bool dark = window.lightnessF() < 0.5;
    const auto mix = [](const QColor &a, const QColor &b, qreal t) {
        return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                                a.greenF() + (b.greenF() - a.greenF()) * t,
                                a.blueF() + (b.blueF() - a.blueF()) * t);
    };
    const QColor body = mix(window, dark ? QColor(Qt::white) : QColor(Qt::black),
                            dark ? 0.10 : 0.06);
    painter.setPen(Qt::NoPen);
    painter.setBrush(body);
    painter.drawRoundedRect(rect().adjusted(0, 1, 0, 0), 5, 5);

    // Title, elided to fit before the X.
    painter.setPen(text);
    const QRect textRect(8, 0, closeRect().left() - 12, height());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                     fontMetrics().elidedText(m_dock->windowTitle(),
                                              Qt::ElideMiddle, textRect.width()));

    // The X: faint grey normally, white on a centered red pill on hover.
    const QRect close = closeRect();
    const QPointF center = QRectF(close).center();
    if (m_hoverClose) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xe0, 0x40, 0x43));
        painter.drawEllipse(center, close.width() / 2.0 - 1.0, close.height() / 2.0 - 1.0);
    }
    QPen pen(m_hoverClose ? QColor(Qt::white) : mix(text, window, 0.35));
    pen.setWidthF(1.3);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    const qreal arm = 3.0;
    painter.drawLine(center + QPointF(-arm, -arm), center + QPointF(arm, arm));
    painter.drawLine(center + QPointF(arm, -arm), center + QPointF(-arm, arm));
}

void DockTitleTab::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && closeRect().contains(event->pos())) {
        m_pressedClose = true;
        event->accept();
        return;
    }
    // Anywhere else: let the QDockWidget have it - that's the native drag.
    event->ignore();
}

void DockTitleTab::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressedClose) {
        m_pressedClose = false;
        if (closeRect().contains(event->pos()))
            emit closeRequested();
        event->accept();
        return;
    }
    event->ignore();
}

void DockTitleTab::mouseMoveEvent(QMouseEvent *event)
{
    const bool hover = closeRect().contains(event->pos());
    if (hover != m_hoverClose) {
        m_hoverClose = hover;
        update();
    }
    if (!m_pressedClose)
        event->ignore(); // keep the native dock drag working
}

void DockTitleTab::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hoverClose) {
        m_hoverClose = false;
        update();
    }
}

bool DockTitleTab::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_dock && event->type() == QEvent::WindowTitleChange)
        update();
    return QWidget::eventFilter(watched, event);
}
