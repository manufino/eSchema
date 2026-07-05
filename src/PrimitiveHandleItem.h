#ifndef PRIMITIVEHANDLEITEM_H
#define PRIMITIVEHANDLEITEM_H

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

class GraphicsPrimitive;

// A small fixed-screen-size grip square representing one control point of the
// selected primitive. Dragging it writes straight back into the primitive's
// control point (through the same grid-snap rule as everything else) - this
// item has no geometry of its own beyond drawing the grip and forwarding
// drags, so one generic class drives resize for every primitive type instead
// of needing a per-primitive resize strategy.
class PrimitiveHandleItem : public QGraphicsItem
{
public:
    PrimitiveHandleItem(GraphicsPrimitive *target, int controlPointIndex, QGraphicsItem *parent = nullptr);

    GraphicsPrimitive *target() const { return m_target; }
    int controlPointIndex() const { return m_index; }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

protected:
    // Dragging is manual (not ItemIsMovable + itemChange) for the same reason
    // as GraphicsPrimitive: automatic item movement does not compose well with
    // ItemIgnoresTransformations here, producing an inconsistent screen-to-scene
    // scale while dragging under zoom. Reading event->scenePos() directly avoids it.
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

private:
    static constexpr qreal HalfSize = 4.0; // screen pixels - ItemIgnoresTransformations keeps this constant at any zoom

    GraphicsPrimitive *m_target;
    int m_index;
};

#endif // PRIMITIVEHANDLEITEM_H
