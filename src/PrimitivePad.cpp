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

    if (m_ri > 0) {
        // Actually punches through whatever was painted underneath (e.g. a
        // trace this pad was dropped on top of) instead of just leaving that
        // circle unpainted, which would let it show through as if the pad
        // had no hole at all. Filling with the sheet's own background color
        // (not a transparent clear - CompositionMode_Clear renders solid
        // black over this view's opaque backing store, which is exactly why
        // buildPath() above uses an even-odd fill instead for hit-testing)
        // is what makes it read as an actual drilled hole.
        const QVariant backgroundSetting = SettingsManager::getInstance().loadSetting("background_color");
        const QColor background = backgroundSetting.isValid() ? QColor(backgroundSetting.toString())
                                                                : QColor(Qt::white);
        painter->setBrush(background);
        painter->drawEllipse(mapFromScene(m_pos), m_ri / 2, m_ri / 2);
    }

    paintLabels(painter);
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
