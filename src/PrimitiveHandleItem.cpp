#include "PrimitiveHandleItem.h"
#include "GraphicsPrimitive.h"
#include "GlobalUtils.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

PrimitiveHandleItem::PrimitiveHandleItem(GraphicsPrimitive *target, int controlPointIndex, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_target(target), m_index(controlPointIndex)
{
    // Deliberately NOT ItemIsMovable - dragging is manual (see header comment).
    setFlags(ItemIgnoresTransformations);
    setZValue(1000); // always drawn on top of ordinary primitives
    setPos(target->controlPoint(controlPointIndex));
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
    event->accept(); // must accept to keep receiving move/release events
}

void PrimitiveHandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    const QPointF snapped = Utils::instance().snapToGrid(event->scenePos());
    setPos(snapped);
    m_target->setControlPoint(m_index, snapped);
}
