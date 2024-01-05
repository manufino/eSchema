#ifndef GRAPHICSPRIMITIVE_H
#define GRAPHICSPRIMITIVE_H

#include <QObject>
#include <QWidget>
#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QApplication>

class GraphicsPrimitive : public QObject, public QGraphicsItem
{
    Q_OBJECT

public:
    explicit GraphicsPrimitive(QGraphicsItem *parent = nullptr);
    ~GraphicsPrimitive();

protected:
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant &value);

signals:

public slots:
    // setta la dimensione della griglia
    void gridSizeChanged(int gridSize);

    // attiva o disattiva il snap sulla griglia
    void snapEnableChanged(bool snap);

    // setta lo stile della penna
    void penStyleIsChanged(Qt::PenStyle penStyle);


private:
    int gridSize;
    Qt::PenStyle penStyle;
    bool snapEnable;
};

#endif // GRAPHICSPRIMITIVE_H
