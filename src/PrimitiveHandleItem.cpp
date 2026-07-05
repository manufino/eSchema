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
    painter->setPen(QPen(Qt::black));
    painter->setBrush(QBrush(Qt::white));
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
