/*
 * RulerWidget.h
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

#ifndef RULERWIDGET_H
#define RULERWIDGET_H

#include <QWidget>

// A Photoshop/Illustrator-style ruler: tick marks and scene-unit labels
// along one edge of the drawing view, plus a thin marker line tracking the
// mouse. The same class serves both the top and left ruler; orientation is
// a runtime property rather than a constructor argument because Qt
// Designer's uic always instantiates a registered custom widget via
// "new ClassName(parent)".
class RulerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RulerWidget(QWidget *parent = nullptr);

    void setOrientation(Qt::Orientation orientation);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    // origin: viewport pixel coordinate (x for a horizontal ruler, y for a
    // vertical one) that scene position 0 currently maps to. scale: viewport
    // pixels per scene unit along this ruler's axis. Together they're the
    // whole linear mapping needed to place ticks, since SheetView never
    // rotates its transform - only pans and zooms.
    void setViewTransform(qreal origin, qreal scale);
    // Scene-unit spacing between minor and major ticks - passed in from
    // SheetView::minorGridStep()/majorGridStep() so the ruler's ticks land
    // exactly on the same lines as the visible drawing grid, rather than
    // using an independently chosen spacing.
    void setGridSteps(qreal minorStep, qreal majorStep);
    // Scene-coordinate position (along this ruler's axis) of the mouse -
    // ignored (marker hidden) once the mouse leaves the drawing view.
    void setMarkerPosition(qreal scenePos, bool visible);

signals:
    // Dragging out of the ruler creates a guide line (see Sheet::Guide):
    // the top ruler drags out a horizontal guide, the left ruler a vertical
    // one. The ruler only reports the gesture in global coordinates -
    // MainWindow owns the mapping onto the drawing view and the guide
    // itself. Moved/finished always arrive between started and the next
    // started, Qt's implicit mouse grab guarantees it.
    void guideDragStarted();
    void guideDragMoved(const QPoint &globalPos);
    void guideDragFinished(const QPoint &globalPos);

protected:
    // Draws the tick marks, unit labels and (when visible) the mouse marker
    // from the current origin/scale mapping.
    void paintEvent(QPaintEvent *event) override;
    // The three mouse overrides implement the guide-drag gesture reported
    // by the signals above: press starts it, moves report the global
    // position, release finishes it.
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Qt::Orientation m_orientation = Qt::Horizontal;
    bool m_draggingGuide = false;
    qreal m_origin = 0.0;
    qreal m_scale = 1.0;
    qreal m_minorStep = 10.0;
    qreal m_majorStep = 50.0;
    qreal m_markerPos = 0.0;
    bool m_markerVisible = false;
};

#endif // RULERWIDGET_H
