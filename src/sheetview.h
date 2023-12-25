#ifndef SHEETVIEW_H
#define SHEETVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QRubberBand>
#include <QMouseEvent>

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

public slots:
    void EnableGrid(bool enable = true) {
        gridEnabled = enable;
        this->scene()->update();
    }

signals:
    void mouseMoved(QPointF point);
    void ZoomLevel(unsigned int level);

private:

    int gridSize;
    float lineGridWidth, lineThickGridWidth;
    QColor lineGridColor, lineThickGridColor, dotsGridColor;
    bool gridEnabled;
    QPoint point;
    QRubberBand *rubberBand;
    QPoint origin;
signals:
    void mousePosChanged();
};

#endif // SHEETVIEW_H
