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

    QPainterPath body;
    body.addRoundedRect(QRectF(5.0, 11.0, 14.0, 10.0), 2.0, 2.0);

    QPainterPath shackle;
    if (locked) {
        // Solid filled body + symmetric closed shackle + a punched-out
        // keyhole - the classic "shut" padlock silhouette, unmistakably
        // different (filled vs outline, closed vs open shackle) from the
        // unlocked icon below rather than a subtle variation of the same
        // shape.
        painter.setBrush(Qt::black);
        painter.drawPath(body);

        shackle.moveTo(8.0, 11.0);
        shackle.lineTo(8.0, 8.0);
        shackle.arcTo(QRectF(8.0, 3.0, 8.0, 10.0), 180.0, -180.0);
        shackle.lineTo(16.0, 11.0);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shackle);

        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QRectF(10.5, 14.0, 3.0, 3.0));
        QPainterPath keySlot;
        keySlot.addRoundedRect(QRectF(11.2, 16.0, 1.6, 3.0), 0.5, 0.5);
        painter.drawPath(keySlot);
    } else {
        // Outline-only body (no fill) + the shackle swung well clear to the
        // side - the classic "open" padlock.
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(body);

        shackle.moveTo(8.0, 11.0);
        shackle.lineTo(8.0, 7.0);
        shackle.arcTo(QRectF(8.0, 1.0, 9.0, 10.0), 180.0, -160.0);
        painter.drawPath(shackle);
    }

    painter.end();
    return pixmap;
}

} // namespace LayerIcons
