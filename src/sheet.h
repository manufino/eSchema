#ifndef SHEET_H
#define SHEET_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QList>
#include <QUndoStack>

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
    // Both are idempotent (adding an already-added/removing an already-removed
    // primitive is a harmless no-op) because QUndoStack::push() calls redo()
    // immediately on a command that may have already applied its effect live
    // (see e.g. CreatePrimitiveCommand).
    void addPrimitive(GraphicsPrimitive *primitive);
    // Destructive removal (deletes the primitive) - for callers that don't
    // need undo, e.g. clearPrimitives()/loading a new file.
    void removePrimitive(GraphicsPrimitive *primitive);
    // Non-destructive removal - the caller becomes responsible for the
    // primitive's lifetime (used by DeletePrimitiveCommand, which keeps it
    // alive so a later undo can hand it back via addPrimitive()).
    void takePrimitive(GraphicsPrimitive *primitive);
    const QList<GraphicsPrimitive*> &primitives() const { return m_primitives; }
    void clearPrimitives();

    QUndoStack *undoStack() { return &m_undoStack; }

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
    QUndoStack m_undoStack;
    qreal m_connectionDiameter = 2.0;
    qreal m_lineWidth = 0.5;
    qreal m_lineWidthCircles = 0.35;
};

#endif // SHEET_H
