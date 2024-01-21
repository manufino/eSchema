#include "GraphicsPrimitive.h"

GraphicsPrimitive::GraphicsPrimitive(PrimitiveTypes primitiveType, QGraphicsItem *parent) : QGraphicsItem(parent)
{
    this->primitiveType = primitiveType;

    penStyle = Qt::SolidLine;

    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsMovable |
            QGraphicsItem::ItemSendsGeometryChanges);
}

GraphicsPrimitive::~GraphicsPrimitive()
{}
