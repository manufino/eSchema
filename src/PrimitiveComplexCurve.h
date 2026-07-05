#ifndef PRIMITIVECOMPLEXCURVE_H
#define PRIMITIVECOMPLEXCURVE_H

#include "GraphicsPrimitive.h"
#include <QVector>
#include <QPainterPath>

// FCD "CV" (void/outline) / "CP" (painted/filled) - a smooth curve interpolating
// an arbitrary number of control points, optionally closed (FIDOSPECS.md 5.6).
// Arrows are only meaningful when the curve is open.
class PrimitiveComplexCurve : public GraphicsPrimitive
{
public:
    static constexpr int MaxVertices = 10000;

    explicit PrimitiveComplexCurve(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return m_vertices.size(); }
    QPointF controlPoint(int index) const override { return m_vertices.at(index); }
    void setControlPoint(int index, const QPointF &scenePos) override;

    void appendVertex(const QPointF &scenePos);
    int vertexCount() const { return m_vertices.size(); }

    bool isClosed() const { return m_closed; }
    void setClosed(bool closed) { m_closed = closed; }

    bool isDegenerate() const override;
    QStringList toTokens() const override;
    bool supportsArrows() const override { return !m_closed; }

private:
    QPainterPath buildSplinePath() const;

    QVector<QPointF> m_vertices;
    bool m_closed = false;
};

#endif // PRIMITIVECOMPLEXCURVE_H
