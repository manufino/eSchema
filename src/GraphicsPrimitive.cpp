#include "GraphicsPrimitive.h"
#include "LayerList.h"
#include "GlobalUtils.h"
#include <QGraphicsScene>

GraphicsPrimitive::GraphicsPrimitive(PrimitiveTypes primitiveType, QGraphicsItem *parent) : QGraphicsItem(parent)
{
    this->primitiveType = primitiveType;

    // These were previously left uninitialized, which left filled/visible/etc. as
    // garbage values on every new primitive.
    penStyle = Qt::SolidLine;
    filled = false;
    showName = true;
    showValue = true;
    visible = true;
    penSize = 1;
    _pen = QPen(Qt::black);
    // objLayer was previously left uninitialized (garbage pointer) - every new
    // primitive now defaults to the master layer, matching what a freshly drawn
    // primitive should belong to until the user assigns a different one.
    objLayer = LayerList::getInstance().getMaster();

    // Deliberately NOT ItemIsMovable - dragging is implemented manually in
    // mousePressEvent/mouseMoveEvent below (see the comment on those overrides
    // in the header for why).
    setFlags(QGraphicsItem::ItemIsSelectable);
}

GraphicsPrimitive::~GraphicsPrimitive()
{}

int GraphicsPrimitive::layerIndex() const
{
    QList<Layer*> *layers = LayerList::getInstance().getList();
    int index = layers ? layers->indexOf(objLayer) : -1;
    // An unassigned/unknown layer falls back to layer 0, matching the FCD spec's
    // rule that an out-of-range layer index is coerced to 0.
    if (index < 0 || index > 15)
        return 0;
    return index;
}

void GraphicsPrimitive::mirror(Qt::Orientation axis, const QPointF &pivot)
{
    const int count = controlPointCount();
    for (int i = 0; i < count; ++i) {
        QPointF p = controlPoint(i);
        if (axis == Qt::Horizontal)
            p.setX(pivot.x() - (p.x() - pivot.x())); // flip across a vertical axis
        else
            p.setY(pivot.y() - (p.y() - pivot.y())); // flip across a horizontal axis
        setControlPoint(i, p);
    }
}

void GraphicsPrimitive::rotate90(const QPointF &pivot)
{
    const int count = controlPointCount();
    for (int i = 0; i < count; ++i) {
        const QPointF p = controlPoint(i);
        const qreal dx = p.x() - pivot.x();
        const qreal dy = p.y() - pivot.y();
        // Standard 90 degree counter-clockwise rotation around the pivot.
        setControlPoint(i, QPointF(pivot.x() - dy, pivot.y() + dx));
    }
}

void GraphicsPrimitive::translateControlPoints(const QPointF &delta)
{
    const int count = controlPointCount();
    for (int i = 0; i < count; ++i)
        setControlPoint(i, controlPoint(i) + delta);
}

void GraphicsPrimitive::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragAnchor = Utils::instance().snapToGrid(event->scenePos());
    // Still needed for click-to-select behaviour (ItemIsSelectable).
    QGraphicsItem::mousePressEvent(event);
}

void GraphicsPrimitive::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || !scene()) {
        QGraphicsItem::mouseMoveEvent(event);
        return;
    }

    const QPointF newAnchor = Utils::instance().snapToGrid(event->scenePos());
    const QPointF delta = newAnchor - m_dragAnchor;
    if (delta.isNull())
        return;
    m_dragAnchor = newAnchor;

    // Move every selected primitive together, not just the one under the
    // cursor, so dragging any one item of a multi-selection moves the group.
    const QList<QGraphicsItem *> selected = scene()->selectedItems();
    if (isSelected() && selected.size() > 1) {
        for (QGraphicsItem *item : selected) {
            if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
                primitive->translateControlPoints(delta);
        }
    } else {
        translateControlPoints(delta);
    }
}
