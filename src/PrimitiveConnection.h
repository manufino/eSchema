#ifndef PRIMITIVECONNECTION_H
#define PRIMITIVECONNECTION_H

#include "GraphicsPrimitive.h"

// FCD "SA" - a connection point (filled dot), drawn at the document's
// `diameterConnection` size (default 2.0, overridable per-document via "FJC C",
// FIDOSPECS.md 5.7/7). No FCJ line; name/value TY labels may follow directly.
class PrimitiveConnection : public GraphicsPrimitive
{
public:
    explicit PrimitiveConnection(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return 1; }
    QPointF controlPoint(int index) const override;
    void setControlPoint(int index, const QPointF &scenePos) override;

    qreal diameter() const { return m_diameter; }
    void setDiameter(qreal diameter) { m_diameter = diameter; }

    bool isDegenerate() const override { return false; } // a connection dot is always meaningful
    QStringList toTokens() const override;
    bool supportsFCJ() const override { return false; }

private:
    QPointF m_pos;
    qreal m_diameter = 2.0;
};

#endif // PRIMITIVECONNECTION_H
