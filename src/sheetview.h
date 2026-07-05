#ifndef SHEETVIEW_H
#define SHEETVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QColor>
#include <QEvent>

#include "Sheet.h"
#include "SettingsManager.h"
#include "GlobalUtils.h"

class PrimitivePlacementController;

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

    // Not owned - the controller is created and owned by MainWindow, which
    // wires it up here since it needs sibling widgets (toolbar, property
    // panel) that SheetView itself has no knowledge of.
    void setPlacementController(PrimitivePlacementController *controller) { m_placementController = controller; }

    // Rounds a scene position to the nearest multiple of the configured
    // snap step (SettingsManager "snap_step"), or returns it unchanged when
    // "snap_enabled" is off. One scene unit == one FidoCadJ grid unit, so the
    // default step of 1 guarantees integer coordinates (FIDOSPECS.md 3).
    QPointF snapToGrid(const QPointF &scenePos) const;

protected:
    void drawBackground (QPainter* painter, const QRectF &rect);
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);

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
    int m_originX, m_originY, gridSize, gridMarkSize, zoomLevel;
    float lineGridWidth, lineThickGridWidth;
    bool gridEnabled;
    QColor lineGridColor, lineThickGridColor, dotsGridColor, backgroundColor;
    QPoint point;
    Utils::GridType gridType;
    PrimitivePlacementController *m_placementController = nullptr;
};

#endif // SHEETVIEW_H
