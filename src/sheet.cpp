#include "Sheet.h"

Sheet::Sheet(QObject *parent) :
    QGraphicsScene(parent)
{
}

void Sheet::addPrimitive(GraphicsPrimitive *primitive)
{
    addItem(primitive);
    m_primitives.append(primitive);
}

void Sheet::removePrimitive(GraphicsPrimitive *primitive)
{
    removeItem(primitive);
    m_primitives.removeOne(primitive);
    delete primitive;
}

void Sheet::clearPrimitives()
{
    // QGraphicsScene::clear() deletes every item it still owns, which covers all
    // primitives here since they were all added via addItem() in addPrimitive().
    clear();
    m_primitives.clear();
    m_connectionDiameter = 2.0;
    m_lineWidth = 0.5;
    m_lineWidthCircles = 0.35;
}
