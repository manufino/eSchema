/*
 * PrimitiveHandleItem.cpp
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

#include "PrimitiveHandleItem.h"
#include "GraphicsPrimitive.h"
#include "GlobalUtils.h"
#include "Sheet.h"
#include "ResizePrimitiveCommand.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

PrimitiveHandleItem::PrimitiveHandleItem(GraphicsPrimitive *target, int controlPointIndex, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_target(target), m_index(controlPointIndex)
{
    // Deliberately NOT ItemIsMovable - dragging is manual (see header comment).
    setFlags(ItemIgnoresTransformations);
    setZValue(1000); // always drawn on top of ordinary primitives
    setPos(target->pointAt(controlPointIndex));
}

QRectF PrimitiveHandleItem::boundingRect() const
{
    return QRectF(-HalfSize, -HalfSize, HalfSize * 2, HalfSize * 2);
}

void PrimitiveHandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    // Matches the reference FidoCadJ editor, which draws every control-point
    // handle as a small solid red square (GraphicPrimitive.drawHandles).
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(Qt::red));
    painter->drawRect(boundingRect());
}

void PrimitiveHandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragStartPos = m_target->pointAt(m_index);
    event->accept(); // must accept to keep receiving move/release events
}

void PrimitiveHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF snapped = Utils::instance().snapToGrid(event->scenePos());
    // Only a geometric corner can be constrained (e.g. aspect-ratio locked) -
    // a label point (index >= controlPointCount()) always drags freely.
    if (m_index < m_target->controlPointCount())
        snapped = m_target->constrainResizePoint(m_index, snapped);
    setPos(snapped);
    m_target->setPointAt(m_index, snapped);
}

void PrimitiveHandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    const QPointF after = m_target->pointAt(m_index);
    if (after == m_dragStartPos)
        return; // plain click, nothing to undo

    if (auto *sheet = qobject_cast<Sheet *>(m_target->scene()))
        sheet->undoStack()->push(new ResizePrimitiveCommand(m_target, m_index, m_dragStartPos, after));
}
