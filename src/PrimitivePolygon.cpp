#include "PrimitivePolygon.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitivePolygon::PrimitivePolygon(QGraphicsItem *parent)
    : GraphicsPrimitive(Polyline, parent)
{
}

QRectF PrimitivePolygon::boundingRect() const
{
    QPolygonF poly;
    for (const QPointF &p : m_vertices)
        poly << mapFromScene(p);
    const qreal margin = penSize + 2;
    return poly.boundingRect().adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitivePolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_vertices.size() < 2)
        return;

    QPen pen(drawColor());
    pen.setStyle(penStyle);
    pen.setWidthF(penSize);
    painter->setPen(pen);
    painter->setBrush(isFilled() ? QBrush(pen.color()) : QBrush(Qt::NoBrush));

    QPolygonF poly;
    for (const QPointF &p : m_vertices)
        poly << mapFromScene(p);
    painter->drawPolygon(poly);

    paintLabels(painter);
}

void PrimitivePolygon::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_vertices[index] = scenePos;
}

void PrimitivePolygon::appendVertex(const QPointF &scenePos)
{
    if (m_vertices.size() >= MaxVertices)
        return;
    prepareGeometryChange();
    m_vertices.append(scenePos);
}

void PrimitivePolygon::removeLastVertex()
{
    if (m_vertices.isEmpty())
        return;
    prepareGeometryChange();
    m_vertices.removeLast();
}

bool PrimitivePolygon::isDegenerate() const
{
    return m_vertices.size() < 2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitivePolygon::toTokens() const
{
    using namespace FidoCadTokenUtils;
    QStringList tokens { isFilled() ? QStringLiteral("PP") : QStringLiteral("PV") };
    for (const QPointF &p : m_vertices)
        tokens << roundIntelligently(p.x()) << roundIntelligently(p.y());
    tokens << QString::number(layerIndex());
    return tokens;
}
