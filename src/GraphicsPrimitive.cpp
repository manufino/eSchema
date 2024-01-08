#include "GraphicsPrimitive.h"

GraphicsPrimitive::GraphicsPrimitive(QGraphicsItem *parent) : QGraphicsItem(parent)
{
    gridSize = SettingsManager::getInstance().loadSetting("grid_step").toInt();
    penStyle = Qt::SolidLine;
    snapEnable = true;

    setFlags(QGraphicsItem::ItemIsSelectable |
            QGraphicsItem::ItemIsMovable |
            QGraphicsItem::ItemSendsGeometryChanges);
}

GraphicsPrimitive::~GraphicsPrimitive()
{}

void GraphicsPrimitive::keyPressEvent(QKeyEvent *event)
{

}

void GraphicsPrimitive::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{

}

QVariant GraphicsPrimitive::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene() && snapEnable) {
        QPointF newPos = value.toPointF();
        if(QApplication::mouseButtons() == Qt::LeftButton &&
                qobject_cast<Sheet*> (scene())){
            //Sheet* customScene = qobject_cast<Sheet*> (scene());

            qreal xV = round(newPos.x()/gridSize)*gridSize;
            qreal yV = round(newPos.y()/gridSize)*gridSize;
            return QPointF(xV, yV);
        }
        else
            return newPos;
    }
    else
        return QGraphicsItem::itemChange(change, value);
}

void GraphicsPrimitive::gridSizeChanged(int gridSize)
{
    this->gridSize = gridSize;
}

void GraphicsPrimitive::snapEnableChanged(bool snap)
{
    snapEnable = snap;
}

void GraphicsPrimitive::penStyleIsChanged(Qt::PenStyle penStyle)
{
    this->penStyle = penStyle;
}
