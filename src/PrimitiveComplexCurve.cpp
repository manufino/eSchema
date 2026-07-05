#include "PrimitiveComplexCurve.h"
#include "PrimitiveArrowUtils.h"
#include "FidoCadTokenUtils.h"
#include <QStyleOptionGraphicsItem>

PrimitiveComplexCurve::PrimitiveComplexCurve(QGraphicsItem *parent)
    : GraphicsPrimitive(Spline, parent)
{
}

// Converts the vertex chain into a smooth path using a Catmull-Rom -> cubic
// Bezier conversion (the standard "spline through points" construction), so the
// curve actually passes through every given point instead of just being guided
// by them the way a plain Bezier's control points would.
QPainterPath PrimitiveComplexCurve::buildSplinePath() const
{
    QPainterPath path;
    const int n = m_vertices.size();
    if (n < 2)
        return path;

    auto at = [this, n](int i) -> QPointF {
        if (m_closed)
            return mapFromScene(m_vertices.at((i % n + n) % n));
        i = qBound(0, i, n - 1);
        return mapFromScene(m_vertices.at(i));
    };

    path.moveTo(at(0));
    const int segments = m_closed ? n : n - 1;
    for (int i = 0; i < segments; ++i) {
        const QPointF p0 = at(i - 1);
        const QPointF p1 = at(i);
        const QPointF p2 = at(i + 1);
        const QPointF p3 = at(i + 2);
        const QPointF c1 = p1 + (p2 - p0) / 6.0;
        const QPointF c2 = p2 - (p3 - p1) / 6.0;
        path.cubicTo(c1, c2, p2);
    }
    if (m_closed)
        path.closeSubpath();
    return path;
}

QRectF PrimitiveComplexCurve::boundingRect() const
{
    const qreal margin = penSize + 4;
    return buildSplinePath().boundingRect().adjusted(-margin, -margin, margin, margin)
            .united(labelBoundingRect());
}

void PrimitiveComplexCurve::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible() || m_vertices.size() < 2)
        return;

    QPen pen(objLayer ? objLayer->color() : QColor(Qt::black));
    pen.setStyle(penStyle);
    pen.setWidthF(penSize);
    painter->setPen(pen);
    painter->setBrush(isFilled() && m_closed ? QBrush(pen.color()) : QBrush(Qt::NoBrush));

    const QPainterPath path = buildSplinePath();
    painter->drawPath(path);

    if (!m_closed) {
        if (arrowAtStart())
            PrimitiveArrowUtils::paintArrow(painter, mapFromScene(m_vertices.first()),
                                            mapFromScene(m_vertices.at(1)),
                                            arrowStyleLimiter(), arrowStyleEmpty(),
                                            arrowLength(), arrowHalfWidth());
        if (arrowAtEnd())
            PrimitiveArrowUtils::paintArrow(painter, mapFromScene(m_vertices.last()),
                                            mapFromScene(m_vertices.at(m_vertices.size() - 2)),
                                            arrowStyleLimiter(), arrowStyleEmpty(),
                                            arrowLength(), arrowHalfWidth());
    }

    paintLabels(painter);
}

void PrimitiveComplexCurve::setControlPoint(int index, const QPointF &scenePos)
{
    prepareGeometryChange();
    m_vertices[index] = scenePos;
}

void PrimitiveComplexCurve::appendVertex(const QPointF &scenePos)
{
    if (m_vertices.size() >= MaxVertices)
        return;
    prepareGeometryChange();
    m_vertices.append(scenePos);
}

void PrimitiveComplexCurve::removeLastVertex()
{
    if (m_vertices.isEmpty())
        return;
    prepareGeometryChange();
    m_vertices.removeLast();
}

bool PrimitiveComplexCurve::isDegenerate() const
{
    return m_vertices.size() < 2 && objName.isEmpty() && objValue.isEmpty();
}

QStringList PrimitiveComplexCurve::toTokens() const
{
    using namespace FidoCadTokenUtils;
    QStringList tokens { isFilled() ? QStringLiteral("CP") : QStringLiteral("CV") };
    tokens << (m_closed ? QStringLiteral("1") : QStringLiteral("0"));
    for (const QPointF &p : m_vertices)
        tokens << roundIntelligently(p.x()) << roundIntelligently(p.y());
    tokens << QString::number(layerIndex());
    return tokens;
}
