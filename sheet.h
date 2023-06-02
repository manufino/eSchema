#ifndef SHEET_H
#define SHEET_H

#include <QGraphicsScene>
#include <QPainter>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>

class Sheet : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Sheet(QObject *parent = 0);
    int getGridSize() const {return this->gridSize;}
    void setGrid(int size, QColor clr);
    void enableGrid(bool enabled) {enab = enabled;};

protected:
    void drawBackground (QPainter* painter, const QRectF &rect) override;

private:
    int gridSize;
    QColor gridColor;
    bool enab;
};

#endif // SHEET_H
