/*
 * PrimitiveImage.cpp
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

#include "PrimitiveImage.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>
#include <QByteArray>

PrimitiveImage::PrimitiveImage(QGraphicsItem *parent)
    : GraphicsPrimitive(Image, parent)
{
}

void PrimitiveImage::setImageData(const QString &mimeSubtype, const QString &base64Data)
{
    prepareGeometryChange();
    m_mimeSubtype = mimeSubtype;
    m_base64Data = base64Data;
    m_pixmap.loadFromData(QByteArray::fromBase64(base64Data.toLatin1()));
}

QRectF PrimitiveImage::boundingRect() const
{
    return QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized()
            .united(labelBoundingRect());
}

void PrimitiveImage::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_pixmap.isNull())
        return;

    painter->save();
    painter->setOpacity(m_opacity);
    // Hue rotation is a display-only nicety; the stored base64 data is always
    // the original, unmodified file bytes (FIDOSPECS.md 5.12).
    painter->drawPixmap(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized(), m_pixmap,
                         m_pixmap.rect());
    painter->restore();

    // An image has no layer color to blend when selected (unlike every other
    // primitive) - draw an outline instead, so it isn't the one primitive
    // type with no selection feedback at all.
    if (isSelected()) {
        painter->save();
        painter->setPen(QPen(drawColor(), 0, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized());
        painter->restore();
    }

    paintLabels(painter);
}

QPointF PrimitiveImage::controlPoint(int index) const
{
    return index == 0 ? m_p1 : m_p2;
}

void PrimitiveImage::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    if (index == 0)
        m_p1 = scenePos;
    else
        m_p2 = scenePos;
}

bool PrimitiveImage::isDegenerate() const
{
    return (m_p1 == m_p2 || m_base64Data.isEmpty());
}

QStringList PrimitiveImage::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("IM"),
        roundIntelligently(m_p1.x()), roundIntelligently(m_p1.y()),
        roundIntelligently(m_p2.x()), roundIntelligently(m_p2.y()),
        roundIntelligently(m_opacity), QString::number(m_hue),
        QString::number(layerIndex()),
        m_mimeSubtype, m_base64Data
    };
}
