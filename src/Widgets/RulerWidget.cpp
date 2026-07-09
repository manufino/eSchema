/*
 * RulerWidget.cpp
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

#include "RulerWidget.h"

#include <QPainter>
#include <cmath>

namespace {
// Thickness of the ruler band, in pixels - kept in sync with the fixed size
// given to the corner widget the two rulers meet at in MainWindow.ui.
constexpr int kBreadth = 22;
// Roughly how many pixels should separate two labelled (major) ticks -
// niceStep() picks a scene-unit step that gets as close to this as possible.
constexpr int kTargetLabelSpacingPx = 60;
}

RulerWidget::RulerWidget(QWidget *parent) : QWidget(parent)
{
    setOrientation(Qt::Horizontal);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void RulerWidget::setOrientation(Qt::Orientation orientation)
{
    m_orientation = orientation;
    if (m_orientation == Qt::Horizontal) {
        setFixedHeight(kBreadth);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        setFixedWidth(kBreadth);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    update();
}

QSize RulerWidget::sizeHint() const
{
    return m_orientation == Qt::Horizontal ? QSize(200, kBreadth) : QSize(kBreadth, 200);
}

QSize RulerWidget::minimumSizeHint() const
{
    return sizeHint();
}

void RulerWidget::setViewTransform(qreal origin, qreal scale)
{
    m_origin = origin;
    m_scale = qMax(scale, 0.0001);
    update();
}

void RulerWidget::setMarkerPosition(qreal scenePos, bool visible)
{
    m_markerPos = scenePos;
    m_markerVisible = visible;
    update();
}

qreal RulerWidget::niceStep(qreal targetPixels) const
{
    const qreal rawStep = targetPixels / m_scale;
    const qreal exponent = std::floor(std::log10(rawStep));
    const qreal base = std::pow(10.0, exponent);
    const qreal fraction = rawStep / base;

    qreal niceFraction;
    if (fraction < 1.5)
        niceFraction = 1.0;
    else if (fraction < 3.0)
        niceFraction = 2.0;
    else if (fraction < 7.0)
        niceFraction = 5.0;
    else
        niceFraction = 10.0;

    return niceFraction * base;
}

void RulerWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const QColor background = palette().window().color();
    const QColor foreground = palette().windowText().color();
    const QColor markerColor = palette().highlight().color();

    painter.fillRect(rect(), background);
    painter.setPen(foreground);

    const bool horizontal = m_orientation == Qt::Horizontal;
    const int length = horizontal ? width() : height();
    const int breadth = horizontal ? height() : width();
    if (length <= 0 || breadth <= 0)
        return;

    const qreal majorStep = niceStep(kTargetLabelSpacingPx);
    const qreal minorStep = majorStep / 5.0;

    // First minorStep-aligned scene coordinate visible at pixel 0.
    const qreal firstScene = std::floor((-m_origin / m_scale) / minorStep) * minorStep;

    QFont font = painter.font();
    font.setPixelSize(qMax(8, breadth / 2));
    painter.setFont(font);

    for (qreal sceneVal = firstScene; ; sceneVal += minorStep) {
        const qreal px = m_origin + sceneVal * m_scale;
        if (px > length + 1)
            break;
        if (px < -1)
            continue;

        // A small epsilon absorbs float drift accumulated over many
        // += minorStep steps, which would otherwise make the comparison
        // against majorStep miss ticks that should count as "major".
        const qreal majorRatio = sceneVal / majorStep;
        const bool isMajor = std::abs(majorRatio - std::round(majorRatio)) < 1e-6;

        const int tickLen = isMajor ? breadth : breadth / 2;
        if (horizontal)
            painter.drawLine(QPointF(px, breadth - tickLen), QPointF(px, breadth));
        else
            painter.drawLine(QPointF(breadth - tickLen, px), QPointF(breadth, px));

        if (isMajor) {
            const QString label = QString::number(qRound(sceneVal));
            if (horizontal) {
                painter.drawText(QRectF(px + 2, 0, 60, breadth), Qt::AlignLeft | Qt::AlignVCenter, label);
            } else {
                painter.save();
                painter.translate(0, px - 2);
                painter.rotate(-90);
                painter.drawText(QRectF(0, 0, 60, breadth), Qt::AlignLeft | Qt::AlignVCenter, label);
                painter.restore();
            }
        }
    }

    if (m_markerVisible) {
        const qreal px = m_origin + m_markerPos * m_scale;
        painter.setPen(QPen(markerColor, 1));
        if (horizontal)
            painter.drawLine(QPointF(px, 0), QPointF(px, breadth));
        else
            painter.drawLine(QPointF(0, px), QPointF(breadth, px));
    }

    painter.setPen(foreground);
    if (horizontal)
        painter.drawLine(0, breadth - 1, length, breadth - 1);
    else
        painter.drawLine(breadth - 1, 0, breadth - 1, length);
}
