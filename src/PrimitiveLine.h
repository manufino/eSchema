#ifndef PRIMITIVELINE_H
#define PRIMITIVELINE_H

#include "GraphicsPrimitive.h"

// FCD "LI" - a straight segment between two endpoints, with optional arrows and
// dash style via the FCJ follow-up line (FIDOSPECS.md 5.1).
class PrimitiveLine : public GraphicsPrimitive
{
public:
    explicit PrimitiveLine(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 2; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsArrows() const override { return true; }

private:
    QPointF m_p1, m_p2;
};

#endif // PRIMITIVELINE_H
