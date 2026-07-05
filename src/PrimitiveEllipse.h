#ifndef PRIMITIVEELLIPSE_H
#define PRIMITIVEELLIPSE_H

#include "GraphicsPrimitive.h"

// FCD "EV" (void/outline) / "EP" (painted/filled) - an ellipse inscribed in the
// bounding box between two opposite corners (FIDOSPECS.md 5.4).
class PrimitiveEllipse : public GraphicsPrimitive
{
public:
    explicit PrimitiveEllipse(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 2; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    bool isDegenerate() const override;
    QStringList toTokens() const override;

private:
    QPointF m_p1, m_p2;
};

#endif // PRIMITIVEELLIPSE_H
