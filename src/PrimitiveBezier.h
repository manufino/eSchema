#ifndef PRIMITIVEBEZIER_H
#define PRIMITIVEBEZIER_H

#include "GraphicsPrimitive.h"

// FCD "BE" - a cubic Bezier curve: 2 endpoints + 2 control points
// (FIDOSPECS.md 5.2). Control point order matches toTokens(): p1, ctrl1, ctrl2, p2.
class PrimitiveBezier : public GraphicsPrimitive
{
public:
    explicit PrimitiveBezier(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 4; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsArrows() const override { return true; }

private:
    QPointF m_points[4];
};

#endif // PRIMITIVEBEZIER_H
