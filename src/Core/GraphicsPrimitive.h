/*
 * GraphicsPrimitive.h
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
#include <QPainterPath>

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
    // update() (not just the assignment) matters as soon as anything can
    // call this on an already-visible primitive - e.g. the properties panel
    // editing an existing selection - rather than only at
    // creation/parse-time, before the first paint.
    void setLayer(Layer *layer) { this->objLayer = layer; update(); }
    // Index (0-15) of this primitive's layer in the global LayerList, clamped to a
    // valid FidoCadJ layer range. Used only at FCD read/write time.
    int layerIndex() const;
    bool isFilled() const { return filled; }
    bool isVisible() const { return visible; }
    bool nameIsVisible() const { return showName; }
    bool valueIsVisible() const { return showValue; }
    QPen pen() const { return this->_pen; }
    // The dash style actually used by paint() - needed by FidoCadWriter,
    // which isn't a subclass and so can't reach the protected field directly.
    Qt::PenStyle lineStyle() const { return penStyle; }

    // The stroke width every line-like primitive (Line/Bezier/Rectangle/
    // Ellipse/Polygon/ComplexCurve) actually draws with: read live from the
    // owning Sheet's document-wide default every time, rather than storing
    // one independently per primitive. Matches the reference FidoCadJ
    // editor, where line width is a single shared value (Globals.lineWidth)
    // consulted at draw time - never a per-primitive property - so the whole
    // document always renders at one consistent width. Falls back to the
    // spec's compiled-in default (0.5) for a primitive not attached to any
    // Sheet, e.g. a macro-library body prototype.
    qreal effectiveLineWidth() const;

    // Extra hit-test padding (scene units), read live from the selection
    // tolerance option, same live-settings pattern as effectiveLineWidth().
    // Added on top of the actual drawn stroke width by every concrete
    // primitive's shape() override, so click/rubber-band selection only
    // responds near what's actually drawn rather than anywhere inside the
    // (often much larger) bounding rect - see strokeOutline() below.
    qreal selectionTolerance() const;

    // prepareGeometryChange() before the assignment matters here (unlike
    // setLayer()) because the label text is part of boundingRect() via
    // labelBoundingRect() - skipping it risks stale un-invalidated regions
    // when a name/value edit grows or shrinks the label.
    void setName(const QString &name) { prepareGeometryChange(); objName = name; update(); }
    void setValue(const QString &value) { prepareGeometryChange(); objValue = value; update(); }

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

    // Called by the generic resize-handle drag (PrimitiveHandleItem) before
    // actually moving a geometric control point (index < controlPointCount()
    // - never called for a label point), with the point it's about to be
    // dragged to. Default is a no-op passthrough (free resize, the existing
    // behavior for every primitive type); a primitive that wants to
    // constrain the drag - e.g. PrimitiveImage's "keep aspect ratio" option,
    // pivoting around the opposite corner - overrides this to adjust the
    // point before it's actually applied.
    virtual QPointF constrainResizePoint(int index, const QPointF &point) const { Q_UNUSED(index); return point; }

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
    // Also includes the name/value label positions below (whichever are
    // currently shown), so a move/resize undo restores those too.
    QVector<QPointF> controlPointSnapshot() const;
    void restoreControlPoints(const QVector<QPointF> &points);

    // Absolute scene position of the name/value label, independent of
    // controlPoint(0) once explicitly moved - matches the reference
    // FidoCadJ editor, where these are extra "virtual points" alongside a
    // primitive's own geometry (e.g. PrimitiveMacro.getNameVirtualPointNumber()/
    // getValueVirtualPointNumber()), draggable on their own rather than being
    // pinned at a fixed offset forever. Lazily defaults to
    // controlPoint(0) + labelOffset(idx) the first time it's read, so an
    // never-customized label still lands exactly where it always used to.
    QPointF nameLabelPos() const;
    QPointF valueLabelPos() const;
    void setNameLabelPos(const QPointF &pos);
    void setValueLabelPos(const QPointF &pos);

    // Every control point, plus the name/value label position(s) above -
    // only for whichever of the two is currently shown (showName/showValue
    // and non-empty), in that order, at the indices right after
    // controlPointCount(). This is what PrimitiveHandleItem/
    // SelectionHandleController/ResizePrimitiveCommand actually address, so
    // a label gets its own drag handle - independently movable, yet still
    // shifting together with the primitive via translateControlPoints()
    // since it's stored as an absolute position - without any of the
    // concrete primitive classes needing their own control-point interface
    // touched at all.
    int totalPointCount() const;
    QPointF pointAt(int index) const;
    void setPointAt(int index, const QPointF &pos);

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

    // The color a primitive should actually draw itself with: its assigned
    // layer's color, blended toward green when selected. Qt draws no default
    // visual difference for a selected QGraphicsItem unless paint() does
    // something itself, so every concrete primitive calls this instead of
    // reading layer()->color() directly - otherwise rubber-band-selecting
    // several primitives at once produces no visible feedback at all, since
    // only a single selection gets resize handles. Matches the reference
    // FidoCadJ editor (GraphicPrimitive.selectLayer()/activateSelectColor(),
    // which blends 60% toward green rather than drawing a separate outline).
    QColor drawColor() const;

protected:
    // Draws name()/value() near controlPoint(0) if set and visible. Every
    // concrete primitive calls this at the end of its own paint() - centralized
    // here since any primitive type can carry a label, not just some.
    void paintLabels(QPainter *painter) const;

    // Turns a bare geometry path (a line/curve/outline with no area of its
    // own) into a hit-test region: a stroke of `strokeWidth` plus
    // 2*selectionTolerance() of padding on each side. Shared by every
    // concrete primitive's shape() override - see selectionTolerance().
    QPainterPath strokeOutline(const QPainterPath &path, qreal strokeWidth) const;

    // Unions the name/value label's on-screen area into a shape() path, so
    // clicking directly on a drawn label (which can sit well outside the
    // primitive's own stroked geometry) still selects the primitive it
    // belongs to - matches boundingRect(), which every concrete primitive
    // already unions labelBoundingRect() into for the same reason.
    QPainterPath withLabelArea(const QPainterPath &path) const;

signals:
    void propertiesChanged(GraphicsPrimitive *primitive);

public slots:

    void penStyleIsChanged(Qt::PenStyle penStyle) { this->penStyle = penStyle; update(); }
    void setIsFilled(bool isFilled) { filled = isFilled; update(); }
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

    // mutable: nameLabelPos()/valueLabelPos() are const but lazily compute
    // and cache their default the first time they're read (see their doc
    // comment above) - a primitive whose label was never dragged still
    // needs somewhere to remember "not customized yet, use the default".
    mutable QPointF m_nameLabelPos;
    mutable QPointF m_valueLabelPos;
    mutable bool m_nameLabelPosSet = false;
    mutable bool m_valueLabelPosSet = false;
};

#endif // GRAPHICSPRIMITIVE_H
