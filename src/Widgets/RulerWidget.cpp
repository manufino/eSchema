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
constexpr int kBreadth = 14;
}

RulerWidget::RulerWidget(QWidget *parent) : QWidget(parent)
{
    setOrientation(Qt::Horizontal);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void RulerWidget::setOrientation(Qt::Orientation orientation)
{
    m_orientation = orientation;
    // Clears whatever fixed-size constraint the *other* orientation left
    // behind - setFixedHeight()/setFixedWidth() below only ever constrain
    // one axis, but each also implicitly sets that axis's min *and* max, so
    // switching orientation without resetting first left the previous
    // orientation's fixed axis still capping this one (the vertical ruler
    // ended up capped to the horizontal ruler's fixed height, rendering as a
    // short, squashed box instead of spanning the drawing area's height).
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
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

void RulerWidget::setGridSteps(qreal minorStep, qreal majorStep)
{
    m_minorStep = qMax(minorStep, 0.0001);
    m_majorStep = qMax(majorStep, m_minorStep);
    update();
}

void RulerWidget::setMarkerPosition(qreal scenePos, bool visible)
{
    m_markerPos = scenePos;
    m_markerVisible = visible;
    update();
}

void RulerWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const QColor background = palette().window().color();
    const QColor markerColor = palette().highlight().color();
    // Deliberately plain gray rather than palette-derived: ticks and labels
    // are secondary/reference marks, not primary UI text, so they should
    // read as muted regardless of the active light/dark stylesheet.
    const QColor tickColor(128, 128, 128);
    const QColor labelColor(110, 110, 110);
    const QColor edgeColor(140, 140, 140);

    painter.fillRect(rect(), background);

    const bool horizontal = m_orientation == Qt::Horizontal;
    const int length = horizontal ? width() : height();
    const int breadth = horizontal ? height() : width();
    if (length <= 0 || breadth <= 0)
        return;

    // Below roughly this many pixels apart, minor ticks (but not the major
    // ones, which always follow the grid's own mark spacing) are skipped so
    // a heavily zoomed-out view doesn't turn the ruler into a solid smear.
    const bool drawMinorTicks = m_minorStep * m_scale >= 3.0;

    // How many minor steps make up one major step - an exact integer since
    // majorGridStep() is always a whole multiple of minorGridStep()
    // (SheetView::majorGridStep()). Ticks/labels are placed by multiplying
    // this out from an integer index rather than repeatedly adding
    // m_minorStep, so every labelled value is an exact multiple of the grid
    // step with no float accumulation drift possible.
    const int minorPerMajor = qMax(1, qRound(m_majorStep / m_minorStep));

    // Index (in minorStep units) of the first tick visible at pixel 0.
    const qint64 firstIndex = static_cast<qint64>(std::floor((-m_origin / m_scale) / m_minorStep));

    QFont font = painter.font();
    font.setPixelSize(qMax(7, breadth - 6));
    painter.setFont(font);

    for (qint64 index = firstIndex; ; ++index) {
        const qreal sceneVal = index * m_minorStep;
        const qreal px = m_origin + sceneVal * m_scale;
        if (px > length + 1)
            break;
        if (px < -1)
            continue;

        const bool isMajor = (index % minorPerMajor) == 0;

        if (isMajor || drawMinorTicks) {
            const int tickLen = isMajor ? breadth : breadth / 2;
            painter.setPen(tickColor);
            if (horizontal)
                painter.drawLine(QPointF(px, breadth - tickLen), QPointF(px, breadth));
            else
                painter.drawLine(QPointF(breadth - tickLen, px), QPointF(breadth, px));
        }

        if (isMajor) {
            const QString label = QString::number(qRound(sceneVal));
            painter.setPen(labelColor);
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

    // Cosmetic (width 0) hairline - a real 1-unit-wide pen was rendering
    // heavier than intended on high-DPI displays.
    painter.setPen(QPen(edgeColor, 0));
    if (horizontal)
        painter.drawLine(0, breadth - 1, length, breadth - 1);
    else
        painter.drawLine(breadth - 1, 0, breadth - 1, length);
}
