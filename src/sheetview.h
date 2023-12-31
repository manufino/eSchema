#ifndef SHEETVIEW_H
#define SHEETVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QRubberBand>
#include <QMouseEvent>

#include "settings_manager.h"

class SheetView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit SheetView(QWidget *parent = nullptr);
    int getGridSize() const {return this->gridSize;}
    void setGrid(int size, QColor clr);
    QPoint getMousePos() { return point;}


protected:
    void drawBackground (QPainter* painter, const QRectF &rect);
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    void loadSettings();

public slots:
    void settingChanged();

    void EnableGrid(bool enable = true) {
        gridEnabled = enable;
        this->scene()->update();
    }

signals:
    void mouseMoved(QPointF point);
    void ZoomLevel(unsigned int level);
    void mousePosChanged();

private:
    int gridSize, gridMarkSize, zoomLevel;
    float lineGridWidth, lineThickGridWidth;
    QColor lineGridColor, lineThickGridColor, dotsGridColor, backgroundColor;
    bool gridEnabled;
    QPoint point;
    QRubberBand *rubberBand;
    QPoint origin;
};

#endif // SHEETVIEW_H
