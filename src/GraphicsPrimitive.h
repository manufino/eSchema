#ifndef GRAPHICSPRIMITIVE_H
#define GRAPHICSPRIMITIVE_H

#include <QObject>
#include <QWidget>
#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QStringList>
#include <QVector>
#include <QHash>

#include "Layer.h"


class GraphicsPrimitive : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    typedef enum {
        Line,
        Rectangle,
        Text,
        Polyline,
        Ellipse,
        Bezier,
        Spline,
        Pad,
        PartLib,
        Connection,
        PcbTrack,
        Image
    } PrimitiveTypes;

    explicit GraphicsPrimitive(PrimitiveTypes primitiveType, QGraphicsItem *parent = nullptr);
    ~GraphicsPrimitive();

    PrimitiveTypes getPrimitiveType() const { return primitiveType; }
    QString name() const { return objName; }
    QString value() const { return objValue; }
    Layer *layer() { return objLayer; }
    void setLayer(Layer *layer) { this->objLayer = layer; }
    // Index (0-15) of this primitive's layer in the global LayerList, clamped to a
    // valid FidoCadJ layer range. Used only at FCD read/write time.
    int layerIndex() const;
    bool isFilled() const { return filled; }
    bool isVisible() const { return visible; }
    bool nameIsVisible() const { return showName; }
    bool valueIsVisible() const { return showValue; }
    QPen pen() const { return this->_pen; }
    // The dash style/width actually used by paint() (constructed fresh from
    // these each call) - needed by FidoCadWriter, which isn't a subclass and
    // so can't reach the protected fields directly.
    Qt::PenStyle lineStyle() const { return penStyle; }
    int lineWidth() const { return penSize; }

    void setName(const QString &name) { objName = name; }
    void setValue(const QString &value) { objValue = value; }

    // FidoCadJ FCJ arrow attributes. Only meaningful for the line-like primitives
    // (LI/BE/CV/CP); the rest simply never read them.
    bool arrowAtStart() const { return m_arrowAtStart; }
    bool arrowAtEnd() const { return m_arrowAtEnd; }
    void setArrowAtStart(bool on) { m_arrowAtStart = on; }
    void setArrowAtEnd(bool on) { m_arrowAtEnd = on; }
    bool arrowStyleLimiter() const { return m_arrowStyleLimiter; }
    bool arrowStyleEmpty() const { return m_arrowStyleEmpty; }
    void setArrowStyleLimiter(bool on) { m_arrowStyleLimiter = on; }
    void setArrowStyleEmpty(bool on) { m_arrowStyleEmpty = on; }
    qreal arrowLength() const { return m_arrowLength; }
    qreal arrowHalfWidth() const { return m_arrowHalfWidth; }
    void setArrowLength(qreal len) { m_arrowLength = len; }
    void setArrowHalfWidth(qreal halfWidth) { m_arrowHalfWidth = halfWidth; }

    virtual QRectF boundingRect() const = 0;

    // --- Control-point interface -------------------------------------------------
    // Shared by both the interactive resize handles and the FidoCadJ reader/writer:
    // every primitive exposes its geometry as an ordered list of scene-coordinate
    // points, with no per-primitive-type resize code needed anywhere else.
    virtual int controlPointCount() const = 0;
    virtual QPointF controlPoint(int index) const = 0;
    virtual void setControlPoint(int index, const QPointF &scenePos) = 0;

    // Mirrors/rotates the primitive's control points around a pivot (scene coords).
    // Default implementation applies the transform to every control point in turn;
    // primitives with extra scalar orientation fields (e.g. PrimitiveMacro's
    // orientation/mirror) override this to also update those fields.
    virtual void mirror(Qt::Orientation axis, const QPointF &pivot);
    virtual void rotate90(const QPointF &pivot);

    // Shifts every control point by `delta`. Used by itemChange() to implement
    // dragging: the item's own pos() is kept pinned at (0,0) forever, and
    // moves are applied directly to the control points instead. This keeps
    // the control points the single source of truth for geometry - if a drag
    // instead changed pos() (QGraphicsItem's default behaviour), the control
    // points would go stale relative to the item's new local origin, silently
    // desyncing resize handles, mirror/rotate, and FidoCadJ serialization.
    void translateControlPoints(const QPointF &delta);

    // Snapshot/restore of every control point, in order. Used by the undo
    // commands for move/resize: both capture an absolute "before" and "after"
    // snapshot and replay one or the other via restoreControlPoints(), rather
    // than reapplying a relative delta - safe to call again even if the drag
    // that originally produced the "after" state already applied it live.
    QVector<QPointF> controlPointSnapshot() const;
    void restoreControlPoints(const QVector<QPointF> &points);

    // True when the primitive carries no visible information (degenerate geometry
    // and no name/value label) and should be silently dropped on save, per the FCD
    // spec's "do not serialize a primitive that carries no information" rule.
    virtual bool isDegenerate() const = 0;

    // The primitive's own FCD instruction line (its code + arguments), NOT
    // including the trailing FCJ/TY follow-up lines - those are cross-cutting and
    // are assembled by FidoCadWriter from the shared base-class attributes above.
    virtual QStringList toTokens() const = 0;

    // Whether this primitive type can carry FCJ arrow attributes (LI/BE/CV/CP) as
    // opposed to only a dash style (RV/RP/EV/EP/PV/PP) or no FCJ at all.
    virtual bool supportsArrows() const { return false; }

    // Whether this primitive type can have an FCJ line at all (SA/PL/PA/MC/IM
    // never do - their name/value TY lines follow the primitive directly).
    virtual bool supportsFCJ() const { return true; }

    // Position of the name (index 0) / value (index 1) label, relative to
    // controlPoint(0). Shared by FidoCadWriter (to place the TY label lines)
    // and paintLabels() below (to draw them on screen) so both agree. Chosen
    // to exactly match FIDOSPECS.md's worked example (11); PrimitiveMacro
    // overrides with a wider offset to match that same example.
    virtual QPointF labelOffset(int labelIndex) const { return QPointF(5, labelIndex == 0 ? 5 : 10); }

    // Extra area needed for the name/value label text, in the same
    // (item-local, effectively scene-equal since pos() is pinned at origin)
    // coordinates as controlPoint(). Empty when there's nothing to draw.
    // Every concrete primitive's boundingRect() must union this in, or the
    // label can be clipped/left un-invalidated when it moves.
    QRectF labelBoundingRect() const;

protected:
    // Draws name()/value() near controlPoint(0) if set and visible. Every
    // concrete primitive calls this at the end of its own paint() - centralized
    // here since any primitive type can carry a label, not just some.
    void paintLabels(QPainter *painter) const;

signals:
    void propertiesChanged(GraphicsPrimitive *primitive);

public slots:

    void penStyleIsChanged(Qt::PenStyle penStyle) { this->penStyle = penStyle; }
    void setPenSize(int newPenSize) { penSize = newPenSize; }
    void setIsFilled(bool isFilled) { filled = isFilled; }
    void setNameVisible(bool visible) { showName = visible; }
    void setValueVisible(bool visible) { showValue = visible; }
    void setVisible(bool visible) { this->visible = visible; }
    void setPen(QPen pen) { this->_pen = pen; }

protected:
    // Dragging is implemented manually (mousePressEvent/mouseMoveEvent) rather
    // than via QGraphicsItem's built-in ItemIsMovable + itemChange interception:
    // that mechanism computes movement through the item's own transform, which
    // does not compose well with control points stored in absolute scene
    // coordinates (position deltas came out inconsistent/erratic under zoom).
    // Reading event->scenePos() directly sidesteps that entirely.
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    Qt::PenStyle penStyle;
    bool filled, showName, showValue, visible;
    PrimitiveTypes primitiveType;
    QString objName;
    QString  objValue;
    Layer *objLayer;
    QPen _pen;
    int penSize;

    QPointF m_dragAnchor; // last mouse scene position seen during an active drag
    // Control-point snapshots (per moved primitive - a multi-selection drag
    // moves them all) as they were at mousePress, so mouseReleaseEvent can
    // push one undo-able MovePrimitiveCommand per primitive that actually moved.
    QHash<GraphicsPrimitive *, QVector<QPointF>> m_dragStartSnapshots;

    bool m_arrowAtStart = false;
    bool m_arrowAtEnd = false;
    bool m_arrowStyleLimiter = false;
    bool m_arrowStyleEmpty = false;
    qreal m_arrowLength = 3.0;
    qreal m_arrowHalfWidth = 1.0;
};

#endif // GRAPHICSPRIMITIVE_H
