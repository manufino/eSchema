#include "GraphicsPrimitive.h"
#include "SettingsManager.h"
#include "LayerList.h"
#include "GlobalUtils.h"

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

    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsMovable |
            QGraphicsItem::ItemSendsGeometryChanges);
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

QVariant GraphicsPrimitive::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene())
        return Utils::instance().snapToGrid(value.toPointF());

    return QGraphicsItem::itemChange(change, value);
}
