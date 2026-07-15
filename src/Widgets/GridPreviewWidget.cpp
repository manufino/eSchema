/*
 * GridPreviewWidget.cpp
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

#include "GridPreviewWidget.h"
#include <QPainter>

GridPreviewWidget::GridPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(110);
}

void GridPreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_backgroundColor);

    const bool drawLines = m_type == 0 || m_type == 2;
    const bool drawDots = m_type == 0 || m_type == 1;
    const int markEvery = qMax(1, m_markStep);

    if (drawLines) {
        for (int x = 0, i = 0; x <= width(); x += m_step, ++i) {
            const bool marked = i % markEvery == 0;
            painter.setPen(QPen(marked ? m_markColor : m_lineColor,
                                marked ? m_markWidth : m_lineWidth));
            painter.drawLine(x, 0, x, height());
        }
        for (int y = 0, i = 0; y <= height(); y += m_step, ++i) {
            const bool marked = i % markEvery == 0;
            painter.setPen(QPen(marked ? m_markColor : m_lineColor,
                                marked ? m_markWidth : m_lineWidth));
            painter.drawLine(0, y, width(), y);
        }
    }
    if (drawDots) {
        painter.setPen(QPen(m_dotColor, 2.0));
        for (int x = 0; x <= width(); x += m_step) {
            for (int y = 0; y <= height(); y += m_step)
                painter.drawPoint(x, y);
        }
    }

    painter.setPen(QPen(palette().mid().color(), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}
