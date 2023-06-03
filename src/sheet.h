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

};

#endif // SHEET_H
