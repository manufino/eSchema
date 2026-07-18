/*
 * SheetView.cpp
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

#include "SheetView.h"
#include "PrimitivePlacementController.h"
#include <QScrollBar>
#include <QPainterPath>
#include <QMimeData>
#include <cmath>

namespace {
// The custom mime type a macro drag from the Libraries panel carries (its
// data is the macro key, UTF-8) - shared, by value, with MacroLibraryTree
// in MainWindowLibraryPanel.cpp, the drag source.
const char MacroMimeType[] = "application/x-eschema-macro";
}

SheetView::SheetView(QWidget *parent) : QGraphicsView(parent)
{
    zoomLevel=100;

    connect(&SettingsManager::getInstance(), &SettingsManager::settingIsChanged,
            this, &SheetView::settingChanged);

    // Scrolling (dragging a scrollbar, arrow keys, rubber-band auto-scroll)
    // changes the scene<->viewport mapping without going through wheelEvent/
    // adjustView() below - only the scrollbars themselves see it directly.
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &SheetView::viewTransformChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &SheetView::viewTransformChanged);

    loadSettings();

    setMouseTracking(true);
    setViewportUpdateMode(ViewportUpdateMode::FullViewportUpdate);
    // Antialiasing is applied by loadSettings() per the "render_antialias"
    // setting (Options > Interface).
    setTransformationAnchor(QGraphicsView::NoAnchor);
    // Rubber-band selection is implemented by hand (see the m_rubber*
    // members) instead of Qt's RubberBandDrag: AutoCAD's window/crossing
    // distinction - drag left-to-right selects only what's fully contained
    // (solid blue), right-to-left also what's merely touched (dashed
    // green) - needs both a per-direction selection mode and a custom
    // rectangle look, neither of which the native mode offers. Clicking
    // directly on a primitive still reaches its own mousePressEvent first
    // (GraphicsPrimitive's manual drag handling), so the band only starts
    // over empty space.
    setDragMode(QGraphicsView::NoDrag);
    // For the Libraries panel's macro drags only - see dragEnterEvent():
    // every other payload (e.g. .fcd/.dxf files) is ignored there so it
    // still bubbles up to MainWindow's window-wide file-drop handling.
    setAcceptDrops(true);
}

void SheetView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QLatin1String(MacroMimeType)))
        event->acceptProposedAction();
    else
        event->ignore(); // not ours - propagate up to MainWindow
}

void SheetView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(QLatin1String(MacroMimeType)))
        event->acceptProposedAction();
    else
        event->ignore();
}

void SheetView::dropEvent(QDropEvent *event)
{
    const QString key = QString::fromUtf8(
            event->mimeData()->data(QLatin1String(MacroMimeType)));
    if (key.isEmpty()) {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    // Snapped exactly like a placement click, so a dropped macro lands on
    // the same positions a clicked one would.
    emit macroDropped(key, placementSnap(event->position().toPoint()));
}

void SheetView::setGrid(int size, QColor clr)
{
    gridSize = size;
    dotsGridColor = clr;

    this->update();
}

void SheetView::drawTracingImage(QPainter *painter, const QRectF &rect)
{
    auto *sheet = qobject_cast<Sheet *>(scene());
    if (!sheet || !sheet->hasBackgroundImage())
        return;

    const QRectF target(sheet->backgroundImageCorner(), sheet->backgroundImageLogicalSize());
    if (!target.intersects(rect))
        return;

    painter->drawImage(target, sheet->backgroundImage());
}

void SheetView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // The configured background must be painted whether or not the grid is
    // shown - hiding the grid used to skip this fill too, visibly changing
    // the canvas color to the view's default instead of just dropping the
    // grid marks.
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(backgroundColor));
    painter->drawRect(rect);

    if (!gridEnabled) {
        drawTracingImage(painter, rect);
        return;
    }

    // gridSize is normally clamped to >= 5 by the Options spinbox, but a
    // hand-edited (or pre-existing, pre-clamp) settings file could still
    // load a 0 here - guard against it since it would otherwise divide by
    // zero below. gridMarkSize is normally clamped to >= 10, same reasoning:
    // markStep collapsing to 0 would turn the thick-line loops below into an
    // infinite loop (the step never advances x/y past rect.right()/bottom()).
    const int step = minorGridStep();
    const int markStep = majorGridStep();

    // Floors toward negative infinity regardless of sign - rect.left()/top()
    // can be negative (e.g. after panning past the scene's own origin with
    // the middle-mouse drag in mouseMoveEvent(), which isn't clamped to the
    // scene rect), where C++'s truncating '%' rounds toward zero instead and
    // would misalign the grid origin by up to one step whenever the visible
    // area crosses into negative coordinates.
    const qreal left = std::floor(rect.left() / step) * step;
    const qreal top = std::floor(rect.top() / step) * step;

    drawTracingImage(painter, rect);

    // LINEE+PUNTI o LINEE
    if(gridType == Utils::GridType::LinesAndDots || gridType == Utils::GridType::Lines)
    {
        QVarLengthArray<QLineF, 100> lines;

        for (qreal x = left; x < rect.right(); x += step)
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = top; y < rect.bottom(); y += step)
            lines.append(QLineF(rect.left(), y, rect.right(), y));

        QVarLengthArray<QLineF, 100> thickLines;

        for (qreal x = left; x < rect.right(); x += markStep)
            thickLines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = top; y < rect.bottom(); y += markStep)
            thickLines.append(QLineF(rect.left(), y, rect.right(), y));

        QPen penHLines(lineGridColor, lineGridWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
        painter->setPen(penHLines);
        painter->drawLines(lines.data(), lines.size());

        painter->setPen(QPen(lineThickGridColor, lineThickGridWidth, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin));
        painter->drawLines(thickLines.data(), thickLines.size());
    }

    // PUNTI o LINEE+PUNTI
    if(gridType == Utils::GridType::Dots || gridType == Utils::GridType::LinesAndDots)
    {
        // Matches the reference FidoCadJ editor's grid rendering
        // (Graphics2DSwing.drawGrid): single-pixel dots once the on-screen
        // pitch gets tight, and below a 3-pixel pitch only every 5th grid
        // point is drawn at all, so a zoomed-out drawing is not buried under
        // its own grid. The pen is cosmetic: its width is in device pixels
        // and ignores the zoom transform - a plain setPen(QColor) would
        // create a 1-scene-unit pen whose dots grow into squares of up to
        // ZOOM_SCALE_MAX pixels when zooming in.
        const qreal pitchPx = step * painter->transform().m11();
        const qreal dotStep = (pitchPx < 3.0) ? step * 5.0 : step;
        QPen dotPen(dotsGridColor, pitchPx >= 16.0 ? 2 : 1);
        dotPen.setCosmetic(true);
        painter->setPen(dotPen);

        const qreal dotLeft = std::floor(rect.left() / dotStep) * dotStep;
        const qreal dotTop = std::floor(rect.top() / dotStep) * dotStep;
        const int columns = int((rect.right() - dotLeft) / dotStep) + 1;
        const int rows = int((rect.bottom() - dotTop) / dotStep) + 1;
        QVector<QPointF> points;
        points.reserve(qMax(0, columns) * qMax(0, rows));
        for (qreal x = dotLeft; x < rect.right(); x += dotStep) {
            for (qreal y = dotTop; y < rect.bottom(); y += dotStep) {
                points.append(QPointF(x, y));
            }
        }
        painter->drawPoints(points.data(), points.size());
    }

}

void SheetView::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawForeground(painter, rect); // Sheet's own foreground

    // The window/crossing rubber band: solid blue while dragging
    // left-to-right (window - only fully contained primitives), dashed
    // green right-to-left (crossing - touched ones too), matching the
    // AutoCAD convention the selection semantics follow.
    if (m_rubberActive) {
        const QRect viewRect = QRect(m_rubberOrigin, m_rubberCurrent).normalized();
        const QPolygonF scenePoly = mapToScene(viewRect);
        const bool windowMode = m_rubberCurrent.x() >= m_rubberOrigin.x();
        const QColor color = windowMode ? QColor(60, 110, 220) : QColor(50, 160, 70);
        QPen pen(color);
        pen.setCosmetic(true);
        if (!windowMode)
            pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        QColor fill = color;
        fill.setAlpha(35);
        painter->setBrush(fill);
        painter->drawPolygon(scenePoly);
        painter->setBrush(Qt::NoBrush);
    }

    auto *sheet = qobject_cast<Sheet *>(scene());
    if (!sheet || sheet->guides().isEmpty())
        return;

    QColor color(SettingsManager::getInstance().loadSetting("guide_color").toString());
    if (!color.isValid())
        color = QColor(0, 170, 255);
    QPen pen(color);
    pen.setCosmetic(true); // hairline whatever the zoom
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    const QList<Sheet::Guide> &guides = sheet->guides();
    for (const Sheet::Guide &guide : guides) {
        if (guide.orientation == Qt::Vertical) {
            if (guide.position >= rect.left() && guide.position <= rect.right())
                painter->drawLine(QLineF(guide.position, rect.top(),
                                         guide.position, rect.bottom()));
        } else {
            if (guide.position >= rect.top() && guide.position <= rect.bottom())
                painter->drawLine(QLineF(rect.left(), guide.position,
                                         rect.right(), guide.position));
        }
    }
}

int SheetView::guideIndexAt(const QPoint &viewPos) const
{
    auto *sheet = qobject_cast<Sheet *>(scene());
    if (!sheet || sheet->guides().isEmpty())
        return -1;
    // A few screen pixels of grab distance, converted to scene units.
    const qreal tolerance = 4.0 / qMax(0.01, transform().m11());
    return sheet->guideNear(mapToScene(viewPos), tolerance);
}

qreal SheetView::guidePositionFor(const Sheet::Guide &guide, const QPoint &viewPos) const
{
    const QPointF snapped = snapToGrid(mapToScene(viewPos));
    return guide.orientation == Qt::Vertical ? snapped.x() : snapped.y();
}

void SheetView::wheelEvent(QWheelEvent *event)
{
    // Optionally (Options > Interface) the bare wheel zooms directly,
    // CAD-style, instead of scrolling - Ctrl+wheel always zooms either way.
    const bool wheelZoomsDirectly =
            SettingsManager::getInstance().loadSetting("wheel_zoom_direct").toBool();
    if (wheelZoomsDirectly || (event->modifiers() & Qt::ControlModifier)) {
        // zoom
        int angle = event->angleDelta().y();
        qreal factor;
        if (angle > 0) {
            factor = 1.1;
        } else {
            factor = 0.9;
        }

        qreal currentScale = transform().m11(); // Get the current scale on the x axis
        qreal newScale = currentScale * factor; // Compute the new scale

        // Zoom factor limits
        if (newScale < ZOOM_SCALE_MIN) {
            factor = ZOOM_SCALE_MIN / currentScale;
        } else if (newScale > ZOOM_SCALE_MAX) {
            factor = ZOOM_SCALE_MAX / currentScale;
        }

        // Keep the drawing point under the cursor pinned there through the
        // zoom, by hand: scale, then translate the transform by however far
        // that point moved. QGraphicsView's own AnchorUnderMouse can't do
        // this reliably here - it works by adjusting the scrollbars, which
        // are clamped to the sceneRect, so near the drawing's edges (or on
        // a small drawing) the anchor silently fails and the view drifts.
        // Panning in this view already goes through the transform for the
        // same reason (see the middle-drag in mouseMoveEvent()).
        const QPointF anchorBefore = mapToScene(event->position().toPoint());
        scale(factor, factor);
        const QPointF anchorAfter = mapToScene(event->position().toPoint());
        translate(anchorAfter.x() - anchorBefore.x(), anchorAfter.y() - anchorBefore.y());

        zoomUpdate();
        emit viewTransformChanged();

    } else {
        QGraphicsView::wheelEvent(event);
    }
}


void SheetView::mouseMoveEvent(QMouseEvent *event)
{
    // map the view coordinates onto the scene
    QPoint origin = mapFromGlobal(QCursor::pos());
    QPointF relativeOrigin = mapToScene(origin);

    // An in-progress guide drag owns the mouse until release. The index is
    // revalidated on every move: "Clear guides" is a menu action (so it can
    // carry a keyboard shortcut) and could empty the list mid-drag.
    if (m_draggedGuide >= 0 && (event->buttons() & Qt::LeftButton)) {
        if (auto *sheet = qobject_cast<Sheet *>(scene())) {
            if (m_draggedGuide >= sheet->guides().size()) {
                m_draggedGuide = -1;
            } else {
                const Sheet::Guide guide = sheet->guides().at(m_draggedGuide);
                sheet->moveGuide(m_draggedGuide, guidePositionFor(guide, event->pos()));
                setCursor(guide.orientation == Qt::Vertical ? Qt::SplitHCursor
                                                            : Qt::SplitVCursor);
            }
        }
        emit mouseMoved(relativeOrigin);
        return;
    }

    if (event->buttons() & Qt::MiddleButton)
    {
        setCursor(Qt::ClosedHandCursor);
        QPointF oldp = mapToScene(m_originX, m_originY);
        QPointF newp = mapToScene(event->pos());
        QPointF translation = newp - oldp;

        translate(translation.x(), translation.y());
        emit viewTransformChanged();

        m_originX = event->position().x();
        m_originY = event->position().y();
    } else {
        setCursor(Qt::ArrowCursor);
    }

    if (m_placementController && m_placementController->isPlacementToolActive()) {
        m_placementController->handleMouseMove(placementSnap(event->pos()));
        emit mouseMoved(relativeOrigin);
        return;
    }

    // The window/crossing rubber band follows the cursor (viewport
    // repainted: the rectangle is drawn in drawForeground()).
    if (m_rubberActive && (event->buttons() & Qt::LeftButton)) {
        m_rubberCurrent = event->pos();
        viewport()->update();
        emit mouseMoved(relativeOrigin);
        return;
    }

    // Hovering near a guide (with no drag in progress) shows the grab
    // cursor, so guides read as adjustable rather than as static decoration.
    if (!event->buttons()) {
        const int hovered = guideIndexAt(event->pos());
        if (hovered >= 0) {
            if (auto *sheet = qobject_cast<Sheet *>(scene())) {
                setCursor(sheet->guides().at(hovered).orientation == Qt::Vertical
                          ? Qt::SplitHCursor : Qt::SplitVCursor);
            }
        }
    }

    // Not placing anything - a leftover object-snap highlight would be
    // misleading (drag snapping keeps its own highlight alive, see
    // PrimitiveHandleItem).
    if (!(event->buttons() & Qt::LeftButton)) {
        if (auto *sheet = qobject_cast<Sheet *>(scene()))
            sheet->clearSnapIndicator();
    }

    emit mouseMoved(relativeOrigin);
    QGraphicsView::mouseMoveEvent(event);
}

void SheetView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_placementController
            && m_placementController->isPlacementToolActive()) {
        m_placementController->handleMousePress(placementSnap(event->pos()));
        return;
    }

    // Grabbing a guide takes precedence over selection: guides are thin,
    // deliberate targets, and this is the only way to adjust one.
    if (event->button() == Qt::LeftButton) {
        const int grabbed = guideIndexAt(event->pos());
        if (grabbed >= 0) {
            m_draggedGuide = grabbed;
            return;
        }
    }

    // Right-click while a drawing tool is active exits back to the Select
    // tool, AutoCAD-style ("right-click ends the command"): the in-progress
    // primitive, if any, is discarded - segments of a Line/PCB-track chain
    // already placed stay - and the toolbar returns to Select. The same
    // click's QContextMenuEvent then finds placement inactive and opens
    // the menu right away, in one gesture.
    if (event->button() == Qt::RightButton && m_placementController
            && m_placementController->isPlacementToolActive()) {
        m_placementController->cancelPlacement();
        return;
    }

    // A bare right-click must never touch the selection: its only job is
    // summoning the context menu (contextMenuEvent below), which acts on
    // the current selection. Forwarded to QGraphicsView, the press would
    // clear the selection whenever it lands on empty canvas - so a
    // right-click on free space next to the selected primitives would
    // deselect them right before the menu opens. Only a left click (or
    // Escape) deselects.
    if (event->button() == Qt::RightButton)
        return;

    if (event->button() == Qt::MiddleButton)
    {
        // Store original position.
        m_originX = event->position().x();
        m_originY = event->position().y();
    }

    // A left press on empty canvas starts the hand-made window/crossing
    // rubber band (see the constructor's dragMode comment). The base call
    // below still runs first-hand for item clicks (selection, drags) and
    // clears the selection on an empty-space press, exactly as before.
    if (event->button() == Qt::LeftButton && !itemAt(event->pos())) {
        m_rubberActive = true;
        m_rubberOrigin = event->pos();
        m_rubberCurrent = event->pos();
    }
    QGraphicsView::mousePressEvent(event);
}

void SheetView::mouseReleaseEvent(QMouseEvent *event)
{
    // Dropping a dragged guide outside the drawing area removes it - the
    // standard "throw it back at the ruler" gesture for deleting a guide.
    // (removeGuide itself bounds-checks, so a mid-drag Clear guides is
    // harmless here.)
    if (m_draggedGuide >= 0) {
        if (auto *sheet = qobject_cast<Sheet *>(scene())) {
            if (!viewport()->rect().contains(event->pos()))
                sheet->removeGuide(m_draggedGuide);
        }
        m_draggedGuide = -1;
        return;
    }

    if (m_placementController && m_placementController->isPlacementToolActive())
        return;

    // Finish the window/crossing rubber band: left-to-right selects only
    // primitives fully contained in the rectangle, right-to-left also the
    // merely touched ones (AutoCAD semantics). Ctrl/Shift extends the
    // current selection instead of replacing it (the empty-space press
    // already cleared it in the plain case).
    if (m_rubberActive && event->button() == Qt::LeftButton) {
        m_rubberActive = false;
        const QRect viewRect = QRect(m_rubberOrigin, m_rubberCurrent).normalized();
        if (viewRect.width() > 3 || viewRect.height() > 3) {
            const bool windowMode = m_rubberCurrent.x() >= m_rubberOrigin.x();
            QPainterPath path;
            path.addPolygon(mapToScene(viewRect));
            path.closeSubpath();
            const QList<QGraphicsItem *> hits = scene()->items(
                        path, windowMode ? Qt::ContainsItemShape
                                         : Qt::IntersectsItemShape);
            const bool extend = event->modifiers()
                    & (Qt::ControlModifier | Qt::ShiftModifier);
            if (!extend)
                scene()->clearSelection();
            for (QGraphicsItem *item : hits) {
                if (item->flags() & QGraphicsItem::ItemIsSelectable)
                    item->setSelected(true);
            }
        }
        viewport()->update();
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void SheetView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_placementController
            && m_placementController->handleMouseDoubleClick(placementSnap(event->pos())))
        return;

    QGraphicsView::mouseDoubleClickEvent(event);
}

void SheetView::contextMenuEvent(QContextMenuEvent *event)
{
    // A placement tool still being active here means the right press was
    // consumed some other way - mousePressEvent normally already exited to
    // Select before this event arrives, letting the menu open below.
    if (m_placementController && m_placementController->isPlacementToolActive())
        return;

    if (auto *primitive = dynamic_cast<GraphicsPrimitive *>(itemAt(event->pos()))) {
        if (!primitive->isSelected()) {
            scene()->clearSelection();
            primitive->setSelected(true);
        }
    }
    emit contextMenuRequested(event->globalPos(), mapToScene(event->pos()));
}

void SheetView::keyPressEvent(QKeyEvent *event)
{
    if (m_placementController && m_placementController->handleKeyPress(event))
        return;

    // Not mid-placement (that case is handled above) - Escape here just
    // clears whatever is currently selected, matching the reference
    // FidoCadJ editor's Escape binding (PopUpMenu ties it to the same
    // "selection" action that also clears the current selection).
    if (event->key() == Qt::Key_Escape && scene()) {
        scene()->clearSelection();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

QPointF SheetView::snapToGrid(const QPointF &scenePos) const
{
    return Utils::instance().snapToGrid(scenePos);
}

QPointF SheetView::placementSnap(const QPoint &viewPos)
{
    const QPointF scenePos = mapToScene(viewPos);
    auto *sheet = qobject_cast<Sheet *>(scene());
    if (!sheet)
        return snapToGrid(scenePos);
    QList<const GraphicsPrimitive *> excluded;
    if (m_placementController && m_placementController->activePrimitive())
        excluded.append(m_placementController->activePrimitive());
    return sheet->snapPosition(scenePos, excluded);
}

void SheetView::loadSettings()
{
    QVariant val = SettingsManager::getInstance().loadSetting("grid_step");
    gridSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("grid_line_width");
    lineGridWidth = val.toDouble();

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_width");
    lineThickGridWidth = val.toDouble();

    val = SettingsManager::getInstance().loadSetting("grid_line_color");
    lineGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_line_mark_color");
    lineThickGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_dot_color");
    dotsGridColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("background_color");
    backgroundColor = QColor(val.toString());

    val = SettingsManager::getInstance().loadSetting("grid_step_mark");
    gridMarkSize = val.toInt();

    val = SettingsManager::getInstance().loadSetting("grid_type");
    gridType = static_cast<Utils::GridType>(val.toInt());

    // Falls back to visible (true) rather than QVariant::toBool()'s own
    // false-for-invalid default, so a settings file saved before this option
    // existed (or none at all yet, on first run) still shows the grid.
    val = SettingsManager::getInstance().loadSetting("grid_visible");
    gridEnabled = val.isValid() ? val.toBool() : true;

    // Rendering quality (Options > Interface) - turning antialiasing off
    // can noticeably speed up very large drawings.
    val = SettingsManager::getInstance().loadSetting("render_antialias");
    setRenderHint(QPainter::Antialiasing, val.isValid() ? val.toBool() : true);
}

void SheetView::zoomUpdate()
{
    // Calcola la percentuale di zoom
    qreal zoomPercentage = (transform().m11() * 100) / ZOOM_SCALE_MAX;
    zoomLevel = qRound(zoomPercentage);

    emit zoomScaleIsChanged(zoomLevel);
}


void SheetView::settingChanged()
{
    loadSettings();
    update();
    // Grid step/mark settings may have just changed - the rulers' tick
    // spacing is derived from them, so nudge MainWindow to re-read it.
    emit viewTransformChanged();
}

void SheetView::adjustView()
{
    fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    zoomUpdate();
    emit viewTransformChanged();
}

void SheetView::adjustViewToSelection()
{
    const QList<QGraphicsItem *> selected = scene()->selectedItems();
    if (selected.isEmpty())
        return;

    QRectF bounds;
    bool first = true;
    for (QGraphicsItem *item : selected) {
        bounds = first ? item->sceneBoundingRect() : bounds.united(item->sceneBoundingRect());
        first = false;
    }
    fitInView(bounds, Qt::KeepAspectRatio);
    zoomUpdate();
    emit viewTransformChanged();
}

void SheetView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    emit viewTransformChanged();
}

void SheetView::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    emit mouseLeftView();
}


