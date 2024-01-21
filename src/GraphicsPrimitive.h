#ifndef GRAPHICSPRIMITIVE_H
#define GRAPHICSPRIMITIVE_H

#include <QObject>
#include <QWidget>
#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QApplication>

#include "Layer.h"


class GraphicsPrimitive : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    typedef enum {
        Line,
        Rectangle,
        Text,
        Polyline,
        Ellipse,
        Bezier,
        Spline,
        Pad,
        PartLib
    } PrimitiveTypes;

    explicit GraphicsPrimitive(PrimitiveTypes primitiveType, QGraphicsItem *parent = nullptr);
    ~GraphicsPrimitive();

    PrimitiveTypes getPrimitiveType() { return primitiveType; }
    QString name() const { return objName; }
    QString value() const { return objValue; }
    Layer *layer() { return objLayer; }
    void setLayer(Layer *layer) { this->objLayer = layer; }
    bool isFilled() const { return filled; }
    bool isVisible() const { return visible; }
    bool nameIsVisible() const { return showName; }
    bool valueIsVisible() const { return showValue; }
    QPen pen() const { return this->_pen; }

    virtual QRectF boundingRect() const = 0;

signals:
    void propertiesChanged(GraphicsPrimitive *primitive);

public slots:

    void penStyleIsChanged(Qt::PenStyle penStyle) { this->penStyle = penStyle; }
    void setPenSize(int newPenSize) { penSize = newPenSize; }
    void setIsFilled(bool isFilled) { filled = isFilled; }
    void setNameVisible(bool visible) { showName = visible; }
    void setValueVisible(bool visible) { showValue = visible; }
    void setVisible(bool visible) { this->visible = visible; }
    void setPen(QPen pen) { this->_pen = pen; }

protected:
    Qt::PenStyle penStyle;
    bool filled, showName, showValue, visible;
    PrimitiveTypes primitiveType;
    QString objName;
    QString  objValue;
    Layer *objLayer;
    QPen _pen;
    int penSize;

};

#endif // GRAPHICSPRIMITIVE_H
