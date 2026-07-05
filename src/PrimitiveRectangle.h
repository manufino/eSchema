#ifndef PRIMITIVERECTANGLE_H
#define PRIMITIVERECTANGLE_H

#include "GraphicsPrimitive.h"

// FCD "RV" (void/outline) / "RP" (painted/filled) - an axis-aligned rectangle
// between two opposite corners; void vs. filled is the existing `filled` flag on
// the base class (FIDOSPECS.md 5.3).
class PrimitiveRectangle : public GraphicsPrimitive
{
public:
    explicit PrimitiveRectangle(QGraphicsItem *parent = nullptr);

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

#endif // PRIMITIVERECTANGLE_H
