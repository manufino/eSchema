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
#include <QImage>

namespace {
// Desaturates every pixel via Qt's own perceptual gray (qGray(), the same
// weighting QImage::convertToFormat(Format_Grayscale8) already uses)
// while preserving alpha - built once and cached, not recomputed per paint.
QPixmap toGrayscale(const QPixmap &source)
{
    QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = line[x];
            const int gray = qGray(pixel);
            line[x] = qRgba(gray, gray, gray, qAlpha(pixel));
        }
    }
    return QPixmap::fromImage(image);
}
}

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
    m_grayscalePixmap = QPixmap(); // stale - rebuilt lazily next paint() if still needed
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

    // Hue rotation and grayscale are display-only; the stored base64 data is
    // always the original, unmodified file bytes (FIDOSPECS.md 5.12).
    if (m_grayscale && m_grayscalePixmap.isNull())
        m_grayscalePixmap = toGrayscale(m_pixmap);
    const QPixmap &displayPixmap = m_grayscale ? m_grayscalePixmap : m_pixmap;

    painter->save();
    painter->setOpacity(m_opacity);
    painter->drawPixmap(QRectF(mapFromScene(m_p1), mapFromScene(m_p2)).normalized(), displayPixmap,
                         displayPixmap.rect());
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

QPointF PrimitiveImage::constrainResizePoint(int index, const QPointF &point) const
{
    if (!m_keepAspectRatio || m_pixmap.isNull())
        return point;

    // Pivots around the opposite, currently-fixed corner - the same corner
    // a free resize would leave untouched.
    const QPointF anchor = index == 0 ? m_p2 : m_p1;
    const qreal naturalRatio = qreal(m_pixmap.width()) / m_pixmap.height();

    QPointF delta = point - anchor;
    // Whichever axis the user is dragging further along (relative to the
    // natural ratio) drives the resize; the other axis is recomputed to
    // match, rather than always deferring to one fixed axis - so dragging
    // mostly sideways resizes by width, mostly up/down resizes by height.
    if (qAbs(delta.x()) > qAbs(delta.y()) * naturalRatio)
        delta.setY((delta.y() < 0 ? -1.0 : 1.0) * qAbs(delta.x()) / naturalRatio);
    else
        delta.setX((delta.x() < 0 ? -1.0 : 1.0) * qAbs(delta.y()) * naturalRatio);
    return anchor + delta;
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
