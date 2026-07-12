/*
 * LayerIcons.cpp
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

#include "LayerIcons.h"
#include <QPainter>
#include <QPainterPath>

namespace LayerIcons {

QPixmap renderLockIcon(bool locked, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Coordinates below are laid out in a 24x24 reference space, then scaled
    // to the requested size - matches the existing 24x24 Remix Icon assets.
    const qreal scale = size / 24.0;
    painter.scale(scale, scale);

    QPen pen(Qt::black, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // Body: a simple rounded rectangle, stroke-only (matches the existing
    // icon set's plain "line" style, e.g. eye-line.png).
    QPainterPath body;
    body.addRoundedRect(QRectF(5.0, 11.0, 14.0, 10.0), 2.0, 2.0);
    painter.drawPath(body);

    // Shackle: an upside-down U. Locked = symmetric, both legs entering the
    // body. Unlocked = the right leg lifted clear of the body - the classic
    // "open padlock" silhouette.
    QPainterPath shackle;
    if (locked) {
        shackle.moveTo(8.0, 11.0);
        shackle.lineTo(8.0, 8.0);
        shackle.arcTo(QRectF(8.0, 3.0, 8.0, 10.0), 180.0, -180.0);
        shackle.lineTo(16.0, 11.0);
    } else {
        shackle.moveTo(7.0, 11.0);
        shackle.lineTo(7.0, 8.0);
        shackle.arcTo(QRectF(7.0, 3.0, 8.0, 10.0), 180.0, -180.0);
        shackle.lineTo(15.0, 6.5);
    }
    painter.drawPath(shackle);

    painter.end();
    return pixmap;
}

} // namespace LayerIcons
