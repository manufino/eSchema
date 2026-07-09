/*
 * PrimitiveText.cpp
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

#include "PrimitiveText.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QFontMetricsF>

PrimitiveText::PrimitiveText(QGraphicsItem *parent)
    : GraphicsPrimitive(Text, parent)
{
}

QRectF PrimitiveText::boundingRect() const
{
    QFont font(m_fontName);
    font.setPointSizeF(qMax(1, m_sizeX));
    const QFontMetricsF metrics(font);
    const QRectF textRect = metrics.boundingRect(m_text);
    return QRectF(mapFromScene(m_pos), textRect.size()).adjusted(-2, -2, 2, 2);
}

void PrimitiveText::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_text.isEmpty())
        return;

    QFont font(m_fontName);
    font.setPointSizeF(qMax(1, m_sizeX));
    font.setBold(m_styleFlags & Bold);
    font.setItalic(m_styleFlags & Italic);
    painter->setFont(font);
    painter->setPen(drawColor());

    painter->save();
    painter->translate(mapFromScene(m_pos));
    painter->rotate(-m_orientationDeg); // FCD orientation is counter-clockwise
    if (m_styleFlags & Mirrored)
        painter->scale(-1, 1);
    // FIDOSPECS anchors a text primitive at the TOP of its bounding box
    // (matches PrimitiveAdvText.java, which tracks the text's extent
    // downward from its anchor by the font ascent), but QPainter::drawText()
    // anchors a point at the BASELINE - without this offset every text
    // primitive rendered one font-ascent too high compared to the reference
    // editor, most noticeable on the small, tightly-packed pin/value labels
    // inside macro bodies.
    painter->drawText(QPointF(0, QFontMetricsF(font).ascent()), m_text);
    painter->restore();
}

QPointF PrimitiveText::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveText::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

void PrimitiveText::mirror(Qt::Orientation axis, const QPointF &pivot)
{
    GraphicsPrimitive::mirror(axis, pivot); // moves the anchor point
    m_styleFlags ^= Mirrored;
}

void PrimitiveText::rotate90(const QPointF &pivot)
{
    GraphicsPrimitive::rotate90(pivot); // moves the anchor point
    m_orientationDeg = (m_orientationDeg + 90) % 360;
}

QStringList PrimitiveText::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("TY"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(m_sizeY), QString::number(m_sizeX),
        QString::number(m_orientationDeg), QString::number(m_styleFlags),
        QString::number(layerIndex()),
        encodeFontName(m_fontName),
        m_text
    };
}
