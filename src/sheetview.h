#ifndef SHEETVIEW_H
#define SHEETVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QRubberBand>
#include <QMouseEvent>
#include <QGraphicsRectItem>

#include "SettingsManager.h"

#define ZOOM_SCALE_MIN 0.3// Scala minima consentita
#define ZOOM_SCALE_MAX 15.0// Scala massima consentita

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
    void mousePressEvent(QMouseEvent* event);

private:
    void loadSettings();
    void zoomUpdate();

public slots:
    void settingChanged();

    void adjustView();

    void enableGrid(bool enable = true) {
        gridEnabled = enable;
        this->scene()->update();
    }

signals:
    void mouseMoved(QPointF point);
    void zoomScaleIsChanged(unsigned int level);
    void mousePosChanged();

private:
    int m_originX, m_originY;
    int gridSize, gridMarkSize, zoomLevel;
    float lineGridWidth, lineThickGridWidth;
    QColor lineGridColor, lineThickGridColor, dotsGridColor, backgroundColor;
    bool gridEnabled;
    QPoint point;
    QRubberBand *rubberBand;
    QPoint origin;
};

#endif // SHEETVIEW_H
