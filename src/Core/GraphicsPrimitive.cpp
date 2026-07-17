/*
 * GraphicsPrimitive.cpp
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

#include "GraphicsPrimitive.h"
#include "DecoratedText.h"
#include "LayerList.h"
#include "GlobalUtils.h"
#include "Sheet.h"
#include "SettingsManager.h"
#include "MovePrimitiveCommand.h"
#include <QGraphicsScene>
#include <QPainterPathStroker>
#include <QFontMetricsF>

GraphicsPrimitive::GraphicsPrimitive(PrimitiveTypes primitiveType, QGraphicsItem *parent) : QGraphicsItem(parent)
{
    this->primitiveType = primitiveType;

    // These were previously left uninitialized, which left filled/visible/etc. as
    // garbage values on every new primitive.
    penStyle = Qt::SolidLine;
    filled = false;
    showName = true;
    showValue = true;
    visible = true;
    // objLayer was previously left uninitialized (garbage pointer) - every new
    // primitive now defaults to the master layer, matching what a freshly drawn
    // primitive should belong to until the user assigns a different one.
    objLayer = LayerList::getInstance().getMaster();

    // Deliberately NOT ItemIsMovable - dragging is implemented manually in
    // mousePressEvent/mouseMoveEvent below (see the comment on those overrides
    // in the header for why).
    setFlags(QGraphicsItem::ItemIsSelectable);
}

GraphicsPrimitive::~GraphicsPrimitive()
{}

qreal GraphicsPrimitive::effectiveLineWidth() const
{
    if (auto *sheet = qobject_cast<Sheet *>(scene()))
        return sheet->lineWidth();
    // Not attached to a Sheet (e.g. a macro-library body prototype, painted
    // directly by PrimitiveMacro rather than living in a real document) -
    // reads the same "line_width" option live, so macro bodies stay in sync
    // with the user's configured default exactly like every other primitive.
    const qreal fromSettings = SettingsManager::getInstance().loadSetting("line_width").toDouble();
    return fromSettings > 0 ? fromSettings : 0.5;
}

qreal GraphicsPrimitive::selectionTolerance() const
{
    const qreal value = SettingsManager::getInstance().loadSetting("selection_tolerance").toDouble();
    return value > 0 ? value : 3.0;
}

QPainterPath GraphicsPrimitive::strokeOutline(const QPainterPath &path, qreal strokeWidth) const
{
    QPainterPathStroker stroker;
    stroker.setWidth(qMax(strokeWidth, 0.0) + 2 * selectionTolerance());
    stroker.setCapStyle(Qt::RoundCap);
    stroker.setJoinStyle(Qt::RoundJoin);
    return stroker.createStroke(path);
}

QPainterPath GraphicsPrimitive::withLabelArea(const QPainterPath &path) const
{
    const QRectF labelRect = labelBoundingRect();
    if (labelRect.isEmpty())
        return path;
    QPainterPath labelPath;
    labelPath.addRect(labelRect);
    return path.united(labelPath);
}

void GraphicsPrimitive::syncLayerAppearance()
{
    // Macros carry no real per-user layer assignment (FIDOSPECS.md 5.10:
    // "MC" tokens have no layer field - MainWindowPropertiesPanel hides the
    // layer field for them for the same reason) - like every FidoCadJ
    // selection/draw path (each explicitly bypasses the layer visible/
    // locked check for "instanceof PrimitiveMacro"), a placed macro is
    // always drawn and always selectable regardless of any layer's
    // visibility or lock state.
    if (primitiveType == PartLib) {
        QGraphicsItem::setVisible(true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        return;
    }

    const bool layerVisible = !objLayer || objLayer->isVisible();
    const bool layerLocked = objLayer && objLayer->isLocked();

    QGraphicsItem::setVisible(layerVisible);
    setFlag(QGraphicsItem::ItemIsSelectable, !layerLocked);

    // ItemIsSelectable only gates *future* selection attempts - it does not
    // retroactively deselect an already-selected item, so a layer hidden or
    // locked out from under an active selection must be dropped explicitly.
    if ((!layerVisible || layerLocked) && isSelected())
        setSelected(false);
}

int GraphicsPrimitive::layerIndex() const
{
    QList<Layer*> *layers = LayerList::getInstance().getList();
    int index = layers ? layers->indexOf(objLayer) : -1;
    // An unassigned/unknown layer falls back to layer 0, matching the FCD spec's
    // rule that an out-of-range layer index is coerced to 0.
    if (index < 0 || index > 15)
        return 0;
    return index;
}

void GraphicsPrimitive::mirror(Qt::Orientation axis, const QPointF &pivot)
{
    const int count = controlPointCount();
    for (int i = 0; i < count; ++i) {
        QPointF p = controlPoint(i);
        if (axis == Qt::Horizontal)
            p.setX(pivot.x() - (p.x() - pivot.x())); // flip across a vertical axis
        else
            p.setY(pivot.y() - (p.y() - pivot.y())); // flip across a horizontal axis
        setControlPoint(i, p);
    }
}

void GraphicsPrimitive::rotate90(const QPointF &pivot)
{
    const int count = controlPointCount();
    for (int i = 0; i < count; ++i) {
        const QPointF p = controlPoint(i);
        const qreal dx = p.x() - pivot.x();
        const qreal dy = p.y() - pivot.y();
        // Standard 90 degree counter-clockwise rotation around the pivot.
        setControlPoint(i, QPointF(pivot.x() - dy, pivot.y() + dx));
    }
}

void GraphicsPrimitive::translateControlPoints(const QPointF &delta)
{
    const int count = controlPointCount();
    for (int i = 0; i < count; ++i)
        setControlPoint(i, controlPoint(i) + delta);
    // Kept in sync unconditionally (not just while shown), so re-enabling
    // showName/showValue later doesn't reveal a label stranded at a stale
    // pre-move position.
    setNameLabelPos(nameLabelPos() + delta);
    setValueLabelPos(valueLabelPos() + delta);
}

QPointF GraphicsPrimitive::nameLabelPos() const
{
    if (!m_nameLabelPosSet) {
        m_nameLabelPos = controlPoint(0) + labelOffset(0);
        m_nameLabelPosSet = true;
    }
    return m_nameLabelPos;
}

QPointF GraphicsPrimitive::valueLabelPos() const
{
    if (!m_valueLabelPosSet) {
        m_valueLabelPos = controlPoint(0) + labelOffset(1);
        m_valueLabelPosSet = true;
    }
    return m_valueLabelPos;
}

void GraphicsPrimitive::setNameLabelPos(const QPointF &pos)
{
    prepareGeometryChange();
    m_nameLabelPos = pos;
    m_nameLabelPosSet = true;
    update();
}

void GraphicsPrimitive::setValueLabelPos(const QPointF &pos)
{
    prepareGeometryChange();
    m_valueLabelPos = pos;
    m_valueLabelPosSet = true;
    update();
}

int GraphicsPrimitive::totalPointCount() const
{
    int extra = 0;
    if (showName && !objName.isEmpty())
        ++extra;
    if (showValue && !objValue.isEmpty())
        ++extra;
    return controlPointCount() + extra;
}

QPointF GraphicsPrimitive::pointAt(int index) const
{
    const int n = controlPointCount();
    if (index < n)
        return controlPoint(index);
    if (showName && !objName.isEmpty())
        return index == n ? nameLabelPos() : valueLabelPos();
    return valueLabelPos(); // only the value label occupies index n here
}

void GraphicsPrimitive::setPointAt(int index, const QPointF &pos)
{
    const int n = controlPointCount();
    if (index < n) {
        setControlPoint(index, pos);
        return;
    }
    if (showName && !objName.isEmpty()) {
        if (index == n) {
            setNameLabelPos(pos);
            return;
        }
        setValueLabelPos(pos);
        return;
    }
    setValueLabelPos(pos);
}

QVector<QPointF> GraphicsPrimitive::controlPointSnapshot() const
{
    const int count = totalPointCount();
    QVector<QPointF> points;
    points.reserve(count);
    for (int i = 0; i < count; ++i)
        points.append(pointAt(i));
    return points;
}

void GraphicsPrimitive::restoreControlPoints(const QVector<QPointF> &points)
{
    for (int i = 0; i < points.size(); ++i)
        setPointAt(i, points.at(i));
}

QColor GraphicsPrimitive::drawColor() const
{
    const QColor base = objLayer ? objLayer->color() : QColor(Qt::black);
    if (!isSelected())
        return base;

    // Blend 60% toward green, same factor as FidoCadJ's activateSelectColor().
    const QColor green(Qt::green);
    const qreal t = 0.6;
    return QColor(qRound(green.red() * t + base.red() * (1 - t)),
                  qRound(green.green() * t + base.green() * (1 - t)),
                  qRound(green.blue() * t + base.blue() * (1 - t)),
                  base.alpha());
}

namespace {

// Shared by labelBoundingRect()/paintLabels(): name/value labels are sized
// and anchored exactly like the reference FidoCadJ editor's
// GraphicPrimitive.drawText(). Its macroFontSize is parsed from the label
// TY line's *x*-size (setValue(): setMacroFontSize(tokens[4])), which is 3
// in standard files, and the em is macroFontSize*12/7 drawing units, the
// stored position being the text's TOP (baseline one ascent below). The
// font family, too, comes from the label TY line (e.g. Bitstream Charter)
// rather than being hardcoded. The font is built at a large fixed pixel
// size and scaled down by the painter: QFont pixel sizes are integers,
// and point sizes resolve against each output device's DPI.
constexpr int LabelBaseFontPixels = 84;

QFont labelBaseFont(const GraphicsPrimitive *primitive)
{
    QFont font(primitive->labelFontName());
    font.setPixelSize(LabelBaseFontPixels);
    return font;
}

qreal labelScale(const GraphicsPrimitive *primitive)
{
    const int sizeX = primitive->labelSizeX() > 0 ? primitive->labelSizeX() : 3;
    return sizeX * 12.0 / 7.0 / LabelBaseFontPixels;
}

}

QRectF GraphicsPrimitive::labelBoundingRect() const
{
    const bool drawName = showName && !objName.isEmpty();
    const bool drawValue = showValue && !objValue.isEmpty();
    // Guards primitives that can transiently have zero control points while
    // still being placed (e.g. a polygon before its first vertex is clicked).
    if ((!drawName && !drawValue) || controlPointCount() == 0)
        return QRectF();

    // Exact per-label measurement (a generous fixed estimate here inflated
    // every labeled primitive's boundingRect, which in turn made
    // fit-to-view leave visible dead margins around the drawing).
    const QFont font = labelBaseFont(this);
    const qreal scale = labelScale(this);
    const qreal ascent = QFontMetricsF(font).ascent();
    const auto labelRect = [&](const QPointF &scenePos, const QString &text) {
        const qreal width = DecoratedText::width(font, text) * scale;
        qreal top = 0, bottom = 0;
        DecoratedText::verticalExtent(font, text, top, bottom);
        return QRectF(mapFromScene(scenePos) + QPointF(0, (ascent + top) * scale),
                      QSizeF(width, (bottom - top) * scale));
    };
    QRectF rect;
    if (drawName)
        rect = rect.united(labelRect(nameLabelPos(), objName));
    if (drawValue)
        rect = rect.united(labelRect(valueLabelPos(), objValue));
    return rect.adjusted(-1, -1, 1, 1);
}

void GraphicsPrimitive::paintLabels(QPainter *painter) const
{
    const bool drawName = showName && !objName.isEmpty();
    const bool drawValue = showValue && !objValue.isEmpty();
    if ((!drawName && !drawValue) || controlPointCount() == 0)
        return;

    painter->save();
    painter->setPen(drawColor());
    // Sized, styled and anchored exactly like the reference FidoCadJ
    // editor's GraphicPrimitive.drawText() - see labelBaseFont()/
    // labelScale() above.
    const QFont font = labelBaseFont(this);
    const qreal scale = labelScale(this);
    const qreal ascent = QFontMetricsF(font).ascent();
    const auto drawLabel = [&](const QPointF &scenePos, const QString &text) {
        painter->save();
        painter->translate(mapFromScene(scenePos));
        painter->scale(scale, scale);
        // DecoratedText: name/value labels support the same ^/_ superscript/
        // subscript markup as standalone text, exactly like the reference
        // editor (GraphicPrimitive.drawText also goes through its
        // DecoratedText). It sets the painter's font itself, per chunk.
        DecoratedText::draw(painter, font, QPointF(0, ascent), text);
        painter->restore();
    };
    if (drawName)
        drawLabel(nameLabelPos(), objName);
    if (drawValue)
        drawLabel(valueLabelPos(), objValue);
    painter->restore();
}

void GraphicsPrimitive::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_dragAnchor = Utils::instance().snapToGrid(event->scenePos());
    m_altDragCloned = false;

    // Snapshot every primitive this drag might move (itself, plus any
    // co-selected siblings), so mouseReleaseEvent can push one undo-able
    // MovePrimitiveCommand per primitive that actually ends up moving.
    m_dragStartSnapshots.clear();
    m_dragStartSnapshots.insert(this, controlPointSnapshot());
    if (scene() && isSelected()) {
        for (QGraphicsItem *item : scene()->selectedItems()) {
            if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item); primitive && primitive != this)
                m_dragStartSnapshots.insert(primitive, primitive->controlPointSnapshot());
        }
    }

    // Still needed for click-to-select behaviour (ItemIsSelectable).
    QGraphicsItem::mousePressEvent(event);
}

void GraphicsPrimitive::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton) || !scene()) {
        QGraphicsItem::mouseMoveEvent(event);
        return;
    }

    const QPointF newAnchor = Utils::instance().snapToGrid(event->scenePos());
    const QPointF delta = newAnchor - m_dragAnchor;
    if (delta.isNull())
        return;
    m_dragAnchor = newAnchor;

    // Alt+drag drags off a duplicate: the moment the drag first actually
    // moves, an in-place copy of the selection is dropped at the original
    // spot (before the translation below), and the drag carries on moving
    // the originals - matching the Illustrator/Inkscape gesture.
    if ((event->modifiers() & Qt::AltModifier) && !m_altDragCloned) {
        m_altDragCloned = true;
        if (auto *sheet = qobject_cast<Sheet *>(scene()))
            sheet->requestAltDragClone();
    }

    // Move every selected primitive together, not just the one under the
    // cursor, so dragging any one item of a multi-selection moves the group.
    const QList<QGraphicsItem *> selected = scene()->selectedItems();
    if (isSelected() && selected.size() > 1) {
        for (QGraphicsItem *item : selected) {
            if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(item))
                primitive->translateControlPoints(delta);
        }
    } else {
        translateControlPoints(delta);
    }
}

void GraphicsPrimitive::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);

    if (m_dragStartSnapshots.isEmpty())
        return;

    auto *sheet = qobject_cast<Sheet *>(scene());
    if (sheet) {
        const bool multiple = m_dragStartSnapshots.size() > 1;
        if (multiple)
            sheet->undoStack()->beginMacro(tr("Move"));
        for (auto it = m_dragStartSnapshots.constBegin(); it != m_dragStartSnapshots.constEnd(); ++it) {
            const QVector<QPointF> after = it.key()->controlPointSnapshot();
            if (after != it.value())
                sheet->undoStack()->push(new MovePrimitiveCommand(it.key(), it.value(), after));
        }
        if (multiple)
            sheet->undoStack()->endMacro();
    }
    m_dragStartSnapshots.clear();
}
