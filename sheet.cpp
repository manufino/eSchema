#include "sheet.h"

Sheet::Sheet(QObject *parent) :
    QGraphicsScene(parent)
{
   // Q_ASSERT(gridSize > 0);
/*
    QSettings s;*/
    gridSize = 10;//s.value("grid_step").toInt();
    gridColor = QColor("black");//s.value("grid_color").value<QColor>();

    enab=true;
}

void Sheet::setGrid(int size, QColor clr)
{
    gridSize = size;
    gridColor = clr;

    this->update();
}

void Sheet::drawBackground(QPainter *painter, const QRectF &rect)
{
    if(enab)
    {
        QPen pen(gridColor);
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

        painter->drawLine(0,0,1000,1000);
    }




}




