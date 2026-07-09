/*
 * SheetView.h
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

#ifndef SHEETVIEW_H
#define SHEETVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QGraphicsRectItem>
#include <QColor>
#include <QEvent>
#include <QContextMenuEvent>

#include "Sheet.h"
#include "SettingsManager.h"
#include "GlobalUtils.h"

class PrimitivePlacementController;

#define ZOOM_SCALE_MIN 0.3// Scala minima consentita
#define ZOOM_SCALE_MAX 15.0// Scala massima consentita

class SheetView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit SheetView(QWidget *parent = nullptr);
    int getGridSize() const {return this->gridSize;}
    void setGrid(int size, QColor clr);
    QPoint getMousePos() { return point;}

    // Not owned - the controller is created and owned by MainWindow, which
    // wires it up here since it needs sibling widgets (toolbar, property
    // panel) that SheetView itself has no knowledge of.
    void setPlacementController(PrimitivePlacementController *controller) { m_placementController = controller; }

    // Rounds a scene position to the nearest multiple of the configured
    // snap step (SettingsManager "snap_step"), or returns it unchanged when
    // "snap_enabled" is off. One scene unit == one FidoCadJ grid unit, so the
    // default step of 1 guarantees integer coordinates (FIDOSPECS.md 3).
    QPointF snapToGrid(const QPointF &scenePos) const;

    // Scene-unit spacing between consecutive minor/major grid lines, exactly
    // as drawn by drawBackground() - RulerWidget reads these so its ticks
    // land on the same lines as the visible grid instead of using an
    // independently chosen spacing.
    qreal minorGridStep() const { return qMax(1, gridSize); }
    qreal majorGridStep() const { return qMax(1, gridMarkSize / 10) * qMax(1, gridSize); }

protected:
    void drawBackground (QPainter* painter, const QRectF &rect);
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    // Right-clicking a not-yet-selected primitive replaces the selection with
    // just that one (so the menu MainWindow builds in response to
    // contextMenuRequested() applies to what was actually clicked) before
    // MainWindow is asked to show it; right-clicking one already part of a
    // multi-selection, or empty canvas, leaves the current selection as-is.
    void contextMenuEvent(QContextMenuEvent *event) override;
    // Both fire viewTransformChanged() (see below): a resize can shift what
    // scene point sits under viewport pixel (0,0) since resizeAnchor
    // defaults to AnchorViewCenter, and the mouse leaving is when the
    // rulers' tracking marker should disappear.
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void loadSettings();
    void zoomUpdate();

public slots:
    void settingChanged();
    void adjustView();

signals:
    void mouseMoved(QPointF point);
    void zoomScaleIsChanged(unsigned int level);
    void mousePosChanged();
    // MainWindow builds and execs the actual QMenu (it owns the ui->action*
    // objects the menu reuses) - SheetView only decides where/on-what the
    // click landed.
    void contextMenuRequested(const QPoint &globalPos);
    // Fired whenever pan, zoom, or resize changes the mapping between scene
    // and viewport coordinates - MainWindow reads transform()/mapFromScene()
    // in response to this to keep the rulers' tick spacing and origin in
    // sync with the view.
    void viewTransformChanged();
    // The mouse pointer has left the viewport - MainWindow uses this to hide
    // the rulers' tracking marker, since no further mouseMoved() will arrive.
    void mouseLeftView();

private:
    int m_originX, m_originY, gridSize, gridMarkSize, zoomLevel;
    float lineGridWidth, lineThickGridWidth;
    bool gridEnabled;
    QColor lineGridColor, lineThickGridColor, dotsGridColor, backgroundColor;
    QPoint point;
    Utils::GridType gridType;
    PrimitivePlacementController *m_placementController = nullptr;
};

#endif // SHEETVIEW_H
