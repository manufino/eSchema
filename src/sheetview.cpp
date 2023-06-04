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

    gridEnabled=true;

    setMouseTracking(true);

    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
}

void SheetView::setGrid(int size, QColor clr)
{
    gridSize = size;
    gridColor = clr;

    this->update();
}

void SheetView::drawBackground(QPainter *painter, const QRectF &rect)
{
    if(gridEnabled)
    {/*
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
        painter->drawPoints(points.data(), points.size());*/

        qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
           qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

           QVarLengthArray<QLineF, 100> lines;

           for (qreal x = left; x < rect.right(); x += gridSize)
               lines.append(QLineF(x, rect.top(), x, rect.bottom()));
           for (qreal y = top; y < rect.bottom(); y += gridSize)
               lines.append(QLineF(rect.left(), y, rect.right(), y));

           QVarLengthArray<QLineF, 100> thickLines;

           for (qreal x = left; x < rect.right(); x += gridSize * 5)
               thickLines.append(QLineF(x, rect.top(), x, rect.bottom()));
           for (qreal y = top; y < rect.bottom(); y += gridSize * 5)
               thickLines.append(QLineF(rect.left(), y, rect.right(), y));

           QPen myPen(Qt::NoPen);
           painter->setBrush(QBrush(QColor(255, 255, 255, 255)));
           painter->setPen(myPen);
           painter->drawRect(rect);

           QPen penHLines(QColor(75, 75, 75), 0.4, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
           painter->setPen(penHLines);
           painter->drawLines(lines.data(), lines.size());

           painter->setPen(QPen(QColor(100, 100, 100), 0.4, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
           painter->drawLines(thickLines.data(), thickLines.size());


           painter->setPen(Qt::blue);

           QVector<QPointF> points;
           for (qreal x = left; x < rect.right(); x += gridSize) {
               for (qreal y = top; y < rect.bottom(); y += gridSize) {
                   points.append(QPointF(x, y));
               }
           }
           painter->drawPoints(points.data(), points.size());
    }
}

int zoomLevel=100;
void SheetView::wheelEvent(QWheelEvent *event)
{
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

        // Limiti del fattore di zoom
        qreal currentScale = transform().m11(); // Ottiene la scala corrente sull'asse x
        qreal newScale = currentScale * factor; // Calcola la nuova scala
        qreal minScale = 0.5; // Scala minima consentita
        qreal maxScale = 2.0; // Scala massima consentita
        if (newScale < minScale) {
            factor = minScale / currentScale;
        } else if (newScale > maxScale) {
            factor = maxScale / currentScale;
        }

        scale(factor, factor);
        setTransformationAnchor(anchor);

        // Calcola la percentuale di zoom
        qreal zoomPercentage = (transform().m11() * 100) / maxScale;
        zoomLevel = qRound(zoomPercentage);
         qDebug(QString::number(zoomLevel).toUtf8());
    } else {
        QGraphicsView::wheelEvent(event);
    }
}


void SheetView::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event);
    QGraphicsView::mouseMoveEvent(event);
}


