#include "sheetview.h"
#include <QSettings>
#include <QColor>
#include "sheet.h"
#include "math.h"
#include <QEvent>


SheetView::SheetView(QWidget *parent) : QGraphicsView(parent)
{
   // QSettings s;
    gridSize = 10;//s.value("grid_step").toInt();
    gridColor = QColor("black");//s.value("grid_color").value<QColor>();

    enab=true;

    setMouseTracking(true);

    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
}

void SheetView::setGrid(int size, QColor clr)
{
    gridSize = size;
    gridColor = clr;

    this->update();
}

void SheetView::drawBackground(QPainter *painter, const QRectF &rect)
{
    QPen pen;
    painter->setPen(pen);

    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);
    QVector<QPointF> points;
    for (qreal x = left; x < rect.right(); x += gridSize){
        for (qreal y = top; y < rect.bottom(); y += gridSize){
            points.append(QPointF(x,y));
        }
    }
    painter->drawPoints(points.data(), points.size());
}

void SheetView::wheelEvent(QWheelEvent *event)
{/*
    if (event->modifiers() & Qt::ControlModifier) {
        // zoom
        const ViewportAnchor anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        int angle = event->angleDelta().y();
        qreal factor;
        if (angle > 0) {
            factor = 1.1;
        } else {
            factor = 0.9;
        }
        scale(factor, factor);
        setTransformationAnchor(anchor);

    } else {
        QGraphicsView::wheelEvent(event);
    }*/
     QGraphicsView::wheelEvent(event);
}

void SheetView::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event);
    QGraphicsView::mouseMoveEvent(event);
}


