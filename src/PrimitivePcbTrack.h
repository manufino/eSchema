#ifndef PRIMITIVEPCBTRACK_H
#define PRIMITIVEPCBTRACK_H

#include "GraphicsPrimitive.h"

// FCD "PL" - a PCB track: a line segment with an explicit width, in grid units
// (FIDOSPECS.md 5.8). No FCJ; name/value TY labels may follow directly.
class PrimitivePcbTrack : public GraphicsPrimitive
{
public:
    explicit PrimitivePcbTrack(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 2; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    qreal width() const { return m_width; }
    void setWidth(qreal width) { m_width = width; }

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    QPointF m_p1, m_p2;
    qreal m_width = 5.0;
};

#endif // PRIMITIVEPCBTRACK_H
