/*
 * sheet.cpp
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

#include "Sheet.h"
#include "SettingsManager.h"
#include "ObjectSnap.h"
#include "GlobalUtils.h"
#include "PrimitivePad.h"
#include "LayerList.h"
#include <QGraphicsView>
#include <QFontMetricsF>
#include <utility>

namespace {
// Falls back to the compiled-in spec default (matches
// GraphicsPrimitive::effectiveLineWidth()'s own fallback) rather than the 0
// QSettings::value() would return for a settings file saved before the
// "line_width" option existed.
qreal defaultLineWidthSetting()
{
    const qreal value = SettingsManager::getInstance().loadSetting("line_width").toDouble();
    return value > 0 ? value : 0.5;
}
// Same fallback pattern, for "connection_diameter" (matches
// PrimitiveConnection::effectiveDiameter()'s own fallback).
qreal defaultConnectionDiameterSetting()
{
    const qreal value = SettingsManager::getInstance().loadSetting("connection_diameter").toDouble();
    return value > 0 ? value : 2.0;
}
}

Sheet::Sheet(QObject *parent) :
    QGraphicsScene(parent)
{
    m_lineWidth = defaultLineWidthSetting();
    m_connectionDiameter = defaultConnectionDiameterSetting();

    connect(&LayerList::getInstance(), &LayerList::layerAppearanceChanged,
            this, &Sheet::refreshLayerAppearance);
    // Deleting a layer from the layer manager must not leave any primitive
    // holding the soon-to-be-freed Layer* - reassign them while it's alive.
    connect(&LayerList::getInstance(), &LayerList::layerAboutToBeRemoved,
            this, &Sheet::reassignLayerBeforeRemoval);
}

void Sheet::reassignLayerBeforeRemoval(Layer *layer)
{
    // The master is the natural fallback home; if the master itself is
    // being removed (or missing), fall back to the first surviving layer.
    Layer *fallback = LayerList::getInstance().getMaster();
    if (!fallback || fallback == layer) {
        fallback = nullptr;
        const QList<Layer *> *layers = LayerList::getInstance().getList();
        for (Layer *candidate : *layers) {
            if (candidate != layer) {
                fallback = candidate;
                break;
            }
        }
    }
    if (!fallback)
        return; // removing the only layer - nothing sane to reassign to

    for (GraphicsPrimitive *primitive : std::as_const(m_primitives)) {
        if (primitive->layer() == layer)
            primitive->setLayer(fallback);
    }
}

void Sheet::addPrimitive(GraphicsPrimitive *primitive)
{
    if (m_primitives.contains(primitive))
        return; // already added - see the idempotency note in Sheet.h
    addItem(primitive);
    // A primitive can be added onto an already-locked/hidden layer (e.g.
    // reloading a file with "FJC K" lines, or a macro body parsed after a
    // lock toggle) - make sure it starts in the right on-screen state.
    primitive->syncLayerAppearance();
    m_primitives.append(primitive);
    if (primitive->getPrimitiveType() == GraphicsPrimitive::Pad)
        m_pads.append(primitive);
    emit primitivesChanged();
}

void Sheet::refreshLayerAppearance()
{
    for (GraphicsPrimitive *primitive : std::as_const(m_primitives))
        primitive->syncLayerAppearance();
    update();
}

void Sheet::takePrimitive(GraphicsPrimitive *primitive)
{
    if (!m_primitives.contains(primitive))
        return; // already removed - see the idempotency note in Sheet.h
    removeItem(primitive);
    m_primitives.removeOne(primitive);
    m_pads.removeOne(primitive);
    emit primitivesChanged();
}

void Sheet::removePrimitive(GraphicsPrimitive *primitive)
{
    // A full no-op (not just a no-op take followed by a delete) when the
    // primitive isn't ours: a caller holding a pointer that a bulk load's
    // clear() already destroyed would otherwise turn a harmless stale call
    // into a double free.
    if (!m_primitives.contains(primitive))
        return;
    takePrimitive(primitive);
    delete primitive;
}

void Sheet::clearPrimitives()
{
    // Drops the selection first so any resize-handle items (owned by
    // SelectionHandleController, outside m_primitives) get torn down through the
    // normal selectionChanged -> clearHandles() path before clear() below deletes
    // their target primitive out from under them - otherwise they're left dangling.
    clearSelection();

    // Must run before m_primitives.clear(): CreatePrimitiveCommand/
    // DeletePrimitiveCommand destructors decide whether they own their primitive
    // by checking whether it's still in m_primitives. Clearing that list first
    // would make every command wrongly conclude it owns (and must delete) a
    // primitive that the QGraphicsScene::clear() below already destroyed,
    // double-freeing it.
    m_undoStack.clear();

    // QGraphicsScene::clear() deletes every item it still owns, which covers all
    // primitives here since they were all added via addItem() in addPrimitive().
    clear();
    m_primitives.clear();
    m_pads.clear();
    m_connectionDiameter = defaultConnectionDiameterSetting();
    m_lineWidth = defaultLineWidthSetting();
    m_lineWidthCircles = 0.35;
    clearBackgroundImage();

    // Layer lock state (persisted per-file as "FJC K", see FidoCadWriter) is
    // process-wide (LayerList is a singleton, not per-Sheet) - reset it here
    // for the same reason as the document config above, so a lock from a
    // previously open document can't leak into whatever gets loaded/created
    // next. Covers every clearPrimitives() caller (New Drawing,
    // FidoCadReader::read(), DxfReader's imports) in one place.
    if (QList<Layer *> *layers = LayerList::getInstance().getList()) {
        for (Layer *layer : std::as_const(*layers))
            layer->setLocked(false);
    }

    emit primitivesChanged();
}

void Sheet::drawForeground(QPainter *painter, const QRectF &)
{
    // Only the pads (tracked separately by addPrimitive/takePrimitive) -
    // scanning every primitive with dynamic_cast here ran on every repaint.
    for (GraphicsPrimitive *primitive : std::as_const(m_pads)) {
        auto *pad = static_cast<PrimitivePad *>(primitive);
        if (pad->isVisible() && pad->isOnCanvas())
            pad->paintHole(painter);
    }

    // Pick highlights for the angular/radial dimension tools: the hovered
    // segment or circle outline (orange, like the snap indicator family)
    // and the already-picked first segment (green). Constant screen width
    // whatever the zoom, same trick as the indicator below.
    if (m_hoverLineVisible || m_hoverEllipseVisible || m_lockedLineVisible) {
        const qreal scale = views().isEmpty() ? 1.0 : views().first()->transform().m11();
        QPen pen(m_hoverLineVisible ? m_hoverLineColor : QColor(255, 140, 0));
        pen.setWidthF(3.0 / qMax(0.01, scale));
        painter->setBrush(Qt::NoBrush);
        if (m_lockedLineVisible) {
            QPen lockedPen(QColor(30, 170, 60));
            lockedPen.setWidthF(3.0 / qMax(0.01, scale));
            painter->setPen(lockedPen);
            painter->drawLine(m_lockedLine);
        }
        painter->setPen(pen);
        if (m_hoverLineVisible)
            painter->drawLine(m_hoverLine);
        if (m_hoverEllipseVisible)
            painter->drawEllipse(m_hoverEllipse);
    }

    // The captured object-snap point, as a small constant-screen-size open
    // square - orange by default, so it can't be confused with the resize
    // handles; recolorable from the Options dialog's Snap page.
    if (m_snapIndicatorVisible) {
        const qreal scale = views().isEmpty() ? 1.0 : views().first()->transform().m11();
        const qreal half = 5.0 / qMax(0.01, scale);
        QColor indicatorColor(SettingsManager::getInstance()
                              .loadSetting("snap_indicator_color").toString());
        if (!indicatorColor.isValid())
            indicatorColor = QColor(255, 128, 0);
        QPen pen(indicatorColor);
        pen.setWidthF(2.0 / qMax(0.01, scale));
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(m_snapIndicator - QPointF(half, half),
                                 QSizeF(half * 2.0, half * 2.0)));
    }

    // Snap tracking: a small cross on every acquired point and a dashed
    // alignment line whenever the cursor is locked onto one - same color
    // family and constant-screen-size trick as the indicator above.
    if (!m_trackingPoints.isEmpty() || !m_trackingLines.isEmpty()) {
        const qreal scale = views().isEmpty() ? 1.0 : views().first()->transform().m11();
        QColor trackColor(SettingsManager::getInstance()
                          .loadSetting("snap_indicator_color").toString());
        if (!trackColor.isValid())
            trackColor = QColor(255, 128, 0);
        QPen crossPen(trackColor);
        crossPen.setWidthF(1.5 / qMax(0.01, scale));
        painter->setPen(crossPen);
        painter->setBrush(Qt::NoBrush);
        const qreal arm = 4.0 / qMax(0.01, scale);
        for (const QPointF &point : std::as_const(m_trackingPoints)) {
            painter->drawLine(QLineF(point - QPointF(arm, 0), point + QPointF(arm, 0)));
            painter->drawLine(QLineF(point - QPointF(0, arm), point + QPointF(0, arm)));
        }
        if (!m_trackingLines.isEmpty()) {
            QPen dashPen(trackColor);
            dashPen.setWidthF(1.0 / qMax(0.01, scale));
            dashPen.setStyle(Qt::DashLine);
            painter->setPen(dashPen);
            for (const QLineF &line : std::as_const(m_trackingLines))
                painter->drawLine(line);
        }
    }

    // Placement tooltip: the live length/angle (or size) readout hugging
    // the cursor while drawing. Painted in device coordinates so neither
    // the box nor its text scales with the zoom; a dark translucent bubble
    // with light text stays readable on any canvas color.
    if (m_placementTooltipVisible && !m_placementTooltipText.isEmpty()) {
        painter->save();
        const QTransform sceneToDevice = painter->worldTransform();
        painter->resetTransform();
        QFont font = painter->font();
        font.setPointSizeF(8.5);
        painter->setFont(font);
        const QFontMetricsF metrics(font);
        const QPointF devicePos = sceneToDevice.map(m_placementTooltipAnchor)
                + QPointF(14.0, 18.0);
        QRectF box = metrics.boundingRect(QRectF(0, 0, 1000, 1000),
                                          Qt::AlignLeft | Qt::AlignTop,
                                          m_placementTooltipText);
        box.moveTopLeft(devicePos);
        box.adjust(-5.0, -3.0, 5.0, 3.0);
        painter->setPen(QPen(QColor(255, 255, 255, 60)));
        painter->setBrush(QColor(45, 45, 45, 215));
        painter->drawRoundedRect(box, 3.0, 3.0);
        painter->setPen(Qt::white);
        painter->drawText(box.adjusted(5.0, 3.0, -5.0, -3.0),
                          Qt::AlignLeft | Qt::AlignTop, m_placementTooltipText);
        painter->restore();
    }
}

QPointF Sheet::snapPosition(const QPointF &scenePos,
                            const QList<const GraphicsPrimitive *> &excluded)
{
    // Capture radius in screen pixels (Options > Snap), whatever the
    // current zoom - shared by object snap and guide snap below.
    const int radiusPx = SettingsManager::getInstance()
            .loadSetting("object_snap_radius").toInt();
    const qreal scale = views().isEmpty() ? 1.0 : views().first()->transform().m11();
    const qreal radius = (radiusPx > 0 ? radiusPx : 12.0) / qMax(0.01, scale);

    const QVariant trackingSetting = SettingsManager::getInstance()
            .loadSetting("object_snap_tracking");
    const bool trackingEnabled = m_objectSnapEnabled
            && (trackingSetting.isValid() ? trackingSetting.toBool() : true);

    if (m_objectSnapEnabled) {
        const std::optional<QPointF> captured =
                ObjectSnap::snapPoint(this, scenePos, radius, excluded);
        if (captured) {
            if (!m_snapIndicatorVisible || m_snapIndicator != *captured) {
                m_snapIndicatorVisible = true;
                m_snapIndicator = *captured;
                update();
            }
            // Snap tracking: hovering a snappable point acquires it - from
            // now on (until this interaction ends) the cursor can align
            // with it from a distance, AutoCAD-style.
            if (trackingEnabled) {
                m_trackingPoints.removeAll(*captured);
                m_trackingPoints.prepend(*captured);
                // A handful is plenty; stale acquisitions just add noise.
                while (m_trackingPoints.size() > 3)
                    m_trackingPoints.removeLast();
            }
            if (!m_trackingLines.isEmpty()) {
                m_trackingLines.clear();
                update();
            }
            return *captured;
        }
    }
    // Hide only the indicator here: this runs on every tracked mouse move,
    // and the acquisitions must survive until the interaction ends
    // (clearSnapIndicator(), which external callers invoke at that point,
    // clears them too).
    if (m_snapIndicatorVisible) {
        m_snapIndicatorVisible = false;
        update();
    }
    QPointF snapped = Utils::instance().snapToGrid(scenePos);

    // Guides snap per axis, each axis to its nearest guide within the
    // radius - so a position can stick to a vertical and a horizontal guide
    // at once (their crossing), overriding the grid on that axis only.
    // Enabled by default; the raw cursor distance (not the grid-snapped
    // one) decides the capture, or a guide between grid lines could never
    // win over the grid.
    const QVariant snapGuidesSetting = SettingsManager::getInstance()
            .loadSetting("snap_to_guides");
    if (!snapGuidesSetting.isValid() || snapGuidesSetting.toBool()) {
        qreal bestX = radius, bestY = radius;
        for (const Guide &guide : std::as_const(m_guides)) {
            if (guide.orientation == Qt::Vertical) {
                const qreal distance = qAbs(scenePos.x() - guide.position);
                if (distance <= bestX) {
                    bestX = distance;
                    snapped.setX(guide.position);
                }
            } else {
                const qreal distance = qAbs(scenePos.y() - guide.position);
                if (distance <= bestY) {
                    bestY = distance;
                    snapped.setY(guide.position);
                }
            }
        }
    }

    // Snap tracking: lock each axis onto the nearest acquired point the raw
    // cursor is aligned with (within the same capture radius), overriding
    // grid/guides on that axis - aligning with two points at once lands on
    // their crossing. The dashed alignment lines drawn from each winning
    // point to the result are what makes the lock legible.
    QList<QLineF> trackingLines;
    if (trackingEnabled && !m_trackingPoints.isEmpty()) {
        const QPointF *winX = nullptr, *winY = nullptr;
        qreal bestX = radius, bestY = radius;
        for (const QPointF &point : std::as_const(m_trackingPoints)) {
            const qreal dx = qAbs(scenePos.x() - point.x());
            if (dx <= bestX) {
                bestX = dx;
                winX = &point;
            }
            const qreal dy = qAbs(scenePos.y() - point.y());
            if (dy <= bestY) {
                bestY = dy;
                winY = &point;
            }
        }
        if (winX)
            snapped.setX(winX->x());
        if (winY)
            snapped.setY(winY->y());
        if (winX && !qFuzzyCompare(winX->y(), snapped.y()))
            trackingLines.append(QLineF(*winX, snapped));
        if (winY && winY != winX && !qFuzzyCompare(winY->x(), snapped.x()))
            trackingLines.append(QLineF(*winY, snapped));
    }
    if (trackingLines != m_trackingLines) {
        m_trackingLines = trackingLines;
        update();
    }
    return snapped;
}

int Sheet::addGuide(Qt::Orientation orientation, qreal position)
{
    m_guides.append({ orientation, position });
    update();
    return m_guides.size() - 1;
}

void Sheet::moveGuide(int index, qreal position)
{
    if (index < 0 || index >= m_guides.size())
        return;
    m_guides[index].position = position;
    update();
}

void Sheet::removeGuide(int index)
{
    if (index < 0 || index >= m_guides.size())
        return;
    m_guides.removeAt(index);
    update();
}

void Sheet::clearGuides()
{
    if (m_guides.isEmpty())
        return;
    m_guides.clear();
    update();
}

int Sheet::guideNear(const QPointF &scenePos, qreal tolerance) const
{
    int best = -1;
    qreal bestDistance = tolerance;
    for (int i = 0; i < m_guides.size(); ++i) {
        const Guide &guide = m_guides.at(i);
        const qreal distance = guide.orientation == Qt::Vertical
                ? qAbs(scenePos.x() - guide.position)
                : qAbs(scenePos.y() - guide.position);
        if (distance <= bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

void Sheet::clearSnapIndicator()
{
    if (!m_snapIndicatorVisible && m_trackingPoints.isEmpty() && m_trackingLines.isEmpty())
        return;
    m_snapIndicatorVisible = false;
    m_trackingPoints.clear();
    m_trackingLines.clear();
    update();
}

void Sheet::setPlacementTooltip(const QPointF &anchor, const QString &text)
{
    if (m_placementTooltipVisible && m_placementTooltipAnchor == anchor
            && m_placementTooltipText == text)
        return;
    m_placementTooltipVisible = true;
    m_placementTooltipAnchor = anchor;
    m_placementTooltipText = text;
    update();
}

void Sheet::clearPlacementTooltip()
{
    if (!m_placementTooltipVisible)
        return;
    m_placementTooltipVisible = false;
    update();
}

void Sheet::setHoverHighlightLine(const QLineF &line, const QColor &color)
{
    m_hoverLine = line;
    m_hoverLineColor = color;
    m_hoverLineVisible = true;
    m_hoverEllipseVisible = false;
    update();
}

void Sheet::setHoverHighlightEllipse(const QRectF &rect)
{
    m_hoverEllipse = rect;
    m_hoverEllipseVisible = true;
    m_hoverLineVisible = false;
    update();
}

void Sheet::setLockedHighlightLine(const QLineF &line)
{
    m_lockedLine = line;
    m_lockedLineVisible = true;
    update();
}

void Sheet::clearHoverHighlight()
{
    if (!m_hoverLineVisible && !m_hoverEllipseVisible)
        return;
    m_hoverLineVisible = false;
    m_hoverEllipseVisible = false;
    update();
}

void Sheet::clearPickHighlights()
{
    if (!m_hoverLineVisible && !m_hoverEllipseVisible && !m_lockedLineVisible)
        return;
    m_hoverLineVisible = false;
    m_hoverEllipseVisible = false;
    m_lockedLineVisible = false;
    update();
}

void Sheet::setBackgroundImage(const QString &mimeSubtype, const QString &base64Data,
                                qreal resolution, const QPointF &corner)
{
    m_backgroundImageMimeSubtype = mimeSubtype;
    m_backgroundImageBase64 = base64Data;
    m_backgroundImage.loadFromData(QByteArray::fromBase64(base64Data.toLatin1()));
    m_backgroundImageResolution = resolution > 0 ? resolution : 200.0;
    m_backgroundImageCorner = corner;
    update();
}

void Sheet::clearBackgroundImage()
{
    m_backgroundImage = QImage();
    m_backgroundImageMimeSubtype.clear();
    m_backgroundImageBase64.clear();
    update();
}

QSizeF Sheet::backgroundImageLogicalSize() const
{
    if (m_backgroundImage.isNull() || m_backgroundImageResolution <= 0)
        return QSizeF();
    constexpr qreal ReferenceDpi = 200.0; // FidoCadJ's logical unit is fixed at 1/200 inch
    const qreal scale = ReferenceDpi / m_backgroundImageResolution;
    return QSizeF(m_backgroundImage.width() * scale, m_backgroundImage.height() * scale);
}
