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
#include <cmath>

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
    // Native rubber-band selection: clicking empty canvas and dragging draws
    // the selection rectangle and selects the primitives it touches. Clicking
    // directly on a primitive still reaches its own mousePressEvent first
    // (GraphicsPrimitive's manual drag handling), so this only kicks in over
    // empty space.
    setDragMode(QGraphicsView::RubberBandDrag);
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

    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(backgroundColor));
    painter->drawRect(rect);

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


void SheetView::wheelEvent(QWheelEvent *event)
{
    // Optionally (Options > Interface) the bare wheel zooms directly,
    // CAD-style, instead of scrolling - Ctrl+wheel always zooms either way.
    const bool wheelZoomsDirectly =
            SettingsManager::getInstance().loadSetting("wheel_zoom_direct").toBool();
    if (wheelZoomsDirectly || (event->modifiers() & Qt::ControlModifier)) {
        // zoom
        const ViewportAnchor anchor = transformationAnchor();
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
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

        scale(factor, factor);
        setTransformationAnchor(anchor);

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

    // Right-click ends a Line/PCB-track chain (matching FidoCadJ) without
    // otherwise opening the default context menu behaviour.
    if (event->button() == Qt::RightButton && m_placementController
            && m_placementController->handleMouseRightClick()) {
        return;
    }

    if (event->button() == Qt::MiddleButton)
    {
        // Store original position.
        m_originX = event->position().x();
        m_originY = event->position().y();
    }
    QGraphicsView::mousePressEvent(event);
}

void SheetView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_placementController && m_placementController->isPlacementToolActive())
        return;

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
    // A placement tool being active means the user is mid-draw - right-click
    // there already means "finish/cancel this chain" (see mousePressEvent
    // above), not "show me a selection menu".
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


