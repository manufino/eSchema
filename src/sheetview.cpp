#include "sheetview.h"
#include <QColor>
#include "sheet.h"
#include "math.h"
#include <QEvent>


SheetView::SheetView(QWidget *parent) : QGraphicsView(parent)
{
    gridEnabled=true;

    zoomLevel=100;

    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &SheetView::settingChanged);

    loadSettings();

    setMouseTracking(true);
    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
}

void SheetView::setGrid(int size, QColor clr)
{
    gridSize = size;
    dotsGridColor = clr;

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

           for (qreal x = left; x < rect.right(); x += gridSize * (gridMarkSize/10))
               thickLines.append(QLineF(x, rect.top(), x, rect.bottom()));
           for (qreal y = top; y < rect.bottom(); y += gridSize * (gridMarkSize/10))
               thickLines.append(QLineF(rect.left(), y, rect.right(), y));

           QPen myPen(Qt::NoPen);
           painter->setBrush(QBrush(backgroundColor));
           painter->setPen(myPen);
           painter->drawRect(rect);

           QPen penHLines(lineGridColor, lineGridWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
           painter->setPen(penHLines);
           painter->drawLines(lines.data(), lines.size());

           painter->setPen(QPen(lineThickGridColor, lineThickGridWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
           painter->drawLines(thickLines.data(), thickLines.size());

           painter->setPen(dotsGridColor);

           QVector<QPointF> points;
           for (qreal x = left; x < rect.right(); x += gridSize) {
               for (qreal y = top; y < rect.bottom(); y += gridSize) {
                   points.append(QPointF(x, y));
               }
           }
           painter->drawPoints(points.data(), points.size());
    }
}


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
        qreal minScale = 0.3; // Scala minima consentita
        qreal maxScale = 15.0; // Scala massima consentita
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
        emit ZoomLevel(zoomLevel);

    } else {
        QGraphicsView::wheelEvent(event);
    }
}


void SheetView::mouseMoveEvent(QMouseEvent *event)
{
    // mappo le coordinate della vista sulla scena
    QPoint origin = mapFromGlobal(QCursor::pos());
    QPointF relativeOrigin = mapToScene(origin);

    emit mouseMoved(relativeOrigin);
    QGraphicsView::mouseMoveEvent(event);
}

void SheetView::loadSettings()
{
    QVariant val = SettingsManager::getInstance().loadSetting("grid_step");
    gridSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("grid_line_width");
    lineGridWidth = val.toDouble();

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_width");
    lineThickGridWidth = val.toDouble();

    val = SettingsManager::getInstance().loadSetting("grid_line_color");
    lineGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_color");
    lineThickGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_dot_color");
    dotsGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("background_color");
    backgroundColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_step_mark");
    gridMarkSize = val.toInt();
}

void SheetView::settingChanged()
{
    loadSettings();
    update();
}


