#ifndef GRAPHICSPRIMITIVE_H
#define GRAPHICSPRIMITIVE_H

#include <QObject>
#include <QWidget>
#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QApplication>

#include "Sheet.h"
#include "SettingsManager.h"


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

class GraphicsPrimitive : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit GraphicsPrimitive(QGraphicsItem *parent = nullptr);
    ~GraphicsPrimitive();

    virtual QRectF boundingRect() const = 0;

protected:
    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant &value);

signals:

public slots:
    void gridSizeChanged(int gridSize);
    void snapEnableChanged(bool snap);
    void penStyleIsChanged(Qt::PenStyle penStyle);


private:
    int gridSize, penSize;
    Qt::PenStyle penStyle;
    bool snapEnable;
};

#endif // GRAPHICSPRIMITIVE_H
