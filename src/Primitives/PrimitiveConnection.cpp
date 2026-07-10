/*
 * PrimitiveConnection.cpp
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

#include "PrimitiveConnection.h"
#include "FidoCadTokenUtils.h"
#include "Sheet.h"
#include "SettingsManager.h"
#include <QStyleOptionGraphicsItem>
#include <QPainterPath>

PrimitiveConnection::PrimitiveConnection(QGraphicsItem *parent)
    : GraphicsPrimitive(Connection, parent)
{
}

qreal PrimitiveConnection::effectiveDiameter() const
{
    if (auto *sheet = qobject_cast<Sheet *>(scene()))
        return sheet->connectionDiameter();
    // Not attached to a Sheet (e.g. a macro-library body prototype) - reads
    // the same "connection_diameter" option live, matching every other
    // primitive not currently in a document (see effectiveLineWidth()).
    const qreal fromSettings = SettingsManager::getInstance().loadSetting("connection_diameter").toDouble();
    return fromSettings > 0 ? fromSettings : 2.0;
}

QRectF PrimitiveConnection::boundingRect() const
{
    const qreal r = effectiveDiameter() / 2.0 + 1;
    return QRectF(mapFromScene(m_pos) - QPointF(r, r), QSizeF(2 * r, 2 * r))
            .united(labelBoundingRect());
}

QPainterPath PrimitiveConnection::shape() const
{
    QPainterPath path;
    const qreal r = effectiveDiameter() / 2.0;
    path.addEllipse(mapFromScene(m_pos), r, r);
    return withLabelArea(path.united(strokeOutline(path, 0)));
}

void PrimitiveConnection::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    const QColor color = drawColor();
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);

    const qreal r = effectiveDiameter() / 2.0;
    painter->drawEllipse(mapFromScene(m_pos), r, r);

    paintLabels(painter);
}

QPointF PrimitiveConnection::controlPoint(int) const
{
    return m_pos;
}

void PrimitiveConnection::setControlPoint(int, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_pos = scenePos;
}

QStringList PrimitiveConnection::toTokens() const
{
    using namespace FidoCadTokenUtils;
    return {
        QStringLiteral("SA"),
        roundIntelligently(m_pos.x()), roundIntelligently(m_pos.y()),
        QString::number(layerIndex())
    };
}
