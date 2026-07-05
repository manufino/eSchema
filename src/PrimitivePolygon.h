#ifndef PRIMITIVEPOLYGON_H
#define PRIMITIVEPOLYGON_H

#include "GraphicsPrimitive.h"
#include <QVector>

// FCD "PV" (void/outline) / "PP" (painted/filled) - a polygon with an arbitrary
// number of vertices, capped at MAX_VERTICES (FIDOSPECS.md 5.5).
class PrimitivePolygon : public GraphicsPrimitive
{
public:
    static constexpr int MaxVertices = 10000;

    explicit PrimitivePolygon(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int controlPointCount() const override { return m_vertices.size(); }
    QPointF controlPoint(int index) const override { return m_vertices.at(index); }
    void setControlPoint(int index, const QPointF &scenePos) override;

    // Used while interactively placing the polygon (one click = one vertex).
    void appendVertex(const QPointF &scenePos);
    void removeLastVertex();
    int vertexCount() const { return m_vertices.size(); }

    bool isDegenerate() const override;
    QStringList toTokens() const override;

private:
    QVector<QPointF> m_vertices;
};

#endif // PRIMITIVEPOLYGON_H
