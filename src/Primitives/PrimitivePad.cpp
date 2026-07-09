/*
 * PrimitivePad.cpp
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

#include "PrimitivePad.h"
#include "FidoCadTokenUtils.h"
#include "SettingsManager.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

PrimitivePad::PrimitivePad(QGraphicsItem *parent)
    : GraphicsPrimitive(Pad, parent)
{
}

QRectF PrimitivePad::boundingRect() const
{
    const qreal margin = 1;
    return QRectF(mapFromScene(m_pos) - QPointF(m_rx / 2, m_ry / 2), QSizeF(m_rx, m_ry))
            .adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

QPainterPath PrimitivePad::buildOuterPath() const
{
    const QPointF center = mapFromScene(m_pos);
    const QRectF outer(center - QPointF(m_rx / 2, m_ry / 2), QSizeF(m_rx, m_ry));

    QPainterPath path;
    switch (m_style) {
    case Round:
        path.addEllipse(outer);
        break;
    case Rectangular:
        path.addRect(outer);
        break;
    case RoundedRectangular:
        path.addRoundedRect(outer, outer.width() * 0.2, outer.height() * 0.2);
        break;
    }
    return path;
}

// The even-odd fill rule punches the hole out of the outer shape - only
// meaningful for hit-testing (shape()) now that paint() below actively
// paints over the hole instead of just leaving it unpainted.
QPainterPath PrimitivePad::buildPath() const
{
    QPainterPath path = buildOuterPath();
    path.setFillRule(Qt::OddEvenFill);
    if (m_ri > 0)
        path.addEllipse(mapFromScene(m_pos), m_ri / 2, m_ri / 2);
    return path;
}

QPainterPath PrimitivePad::shape() const
{
    const QPainterPath path = buildPath();
    return withLabelArea(path.united(strokeOutline(path, 0)));
}

void PrimitivePad::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    painter->setPen(Qt::NoPen);
    painter->setBrush(drawColor());
    painter->drawPath(buildOuterPath());

    // Punches through whatever was painted underneath so far (e.g. a trace
    // this pad was just dropped on top of) - but Sheet::drawForeground()
    // repaints every pad's hole again after every primitive has been drawn,
    // which is what actually keeps it clean against anything drawn *after*
    // the pad too; this call only covers "underneath" for a pad that, for
    // whatever reason, gets painted outside of a Sheet (there is no such
    // case today, but paint() shouldn't depend on drawForeground() alone to
    // look correct).
    paintHole(painter);

    paintLabels(painter);
}

void PrimitivePad::paintHole(QPainter *painter) const
{
    if (m_ri <= 0)
        return;

    // Filling with the sheet's own background color (not a transparent
    // clear - CompositionMode_Clear renders solid black over this view's
    // opaque backing store, which is exactly why buildPath() uses an
    // even-odd fill instead for hit-testing) is what makes this read as an
    // actual drilled hole rather than a see-through gap.
    const QVariant backgroundSetting = SettingsManager::getInstance().loadSetting("background_color");
    const QColor background = backgroundSetting.isValid() ? QColor(backgroundSetting.toString())
                                                            : QColor(Qt::white);
    painter->setPen(Qt::NoPen);
    painter->setBrush(background);
    // controlPoint(0) (== m_pos) rather than mapFromScene(m_pos): numerically
    // identical here since pos() is always pinned at the origin (see
    // GraphicsPrimitive's header comment), but this makes it explicit that
    // the coordinate is scene-space, since Sheet::drawForeground() calls
    // this with a painter that has no per-item transform at all.
    painter->drawEllipse(controlPoint(0), m_ri / 2, m_ri / 2);
}

QPointF PrimitivePad::controlPoint(int) const
{
    return m_pos;
}

void PrimitivePad::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

QStringList PrimitivePad::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("PA"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        roundIntelligently(m_rx), roundIntelligently(m_ry), roundIntelligently(m_ri),
        QString::number(int(m_style)),
        QString::number(layerIndex())
    };
}
