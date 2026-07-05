#ifndef SHEET_H
#define SHEET_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QList>

#include "GraphicsPrimitive.h"

class Sheet : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit Sheet(QObject *parent = 0);

    // Primitives in insertion/document order. QGraphicsScene::items() is
    // returned in an unspecified (roughly z-order) order, but FidoCadJ
    // round-trip idempotency (FIDOSPECS.md 9) requires a stable document order,
    // so Sheet keeps its own ordered list alongside the QGraphicsScene.
    void addPrimitive(GraphicsPrimitive *primitive);
    void removePrimitive(GraphicsPrimitive *primitive);
    const QList<GraphicsPrimitive*> &primitives() const { return m_primitives; }
    void clearPrimitives();

    // Document-wide FidoCadJ settings (FIDOSPECS.md 7's "FJC C/A/B"), each
    // defaulting to the spec's compiled-in default. FidoCadReader sets these
    // from an "FJC" line; FidoCadWriter re-emits one only when it differs from
    // the default, so these round-trip instead of being silently dropped.
    qreal connectionDiameter() const { return m_connectionDiameter; }
    void setConnectionDiameter(qreal diameter) { m_connectionDiameter = diameter; }
    qreal lineWidth() const { return m_lineWidth; }
    void setLineWidth(qreal width) { m_lineWidth = width; }
    qreal lineWidthCircles() const { return m_lineWidthCircles; }
    void setLineWidthCircles(qreal width) { m_lineWidthCircles = width; }

private:
    QList<GraphicsPrimitive*> m_primitives;
    qreal m_connectionDiameter = 2.0;
    qreal m_lineWidth = 0.5;
    qreal m_lineWidthCircles = 0.35;
};

#endif // SHEET_H
