/*
 * DialogArray.h
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

#ifndef DIALOGARRAY_H
#define DIALOGARRAY_H

#include <QDialog>
#include <QPointF>

namespace Ui { class DialogArray; }
class SheetView;

// Parameters for Edit > Array of copies. Two layouts: a columns x rows grid
// with configurable steps, or a circular (polar) series of instances around
// a center - AutoCAD-style: `copies()` total instances spread over
// `totalAngle()` degrees, each optionally rotated to follow the circle.
// The circular center can be typed in, or picked directly on the canvas
// ("Pick from canvas" hides the dialog until the next click - see
// eventFilter()).
class DialogArray : public QDialog
{
    Q_OBJECT

public:
    enum class Mode { Grid, Circular };

    // `view` is the canvas used by the center-picking button; the dialog
    // does not take ownership.
    explicit DialogArray(SheetView *view, QWidget *parent = nullptr);
    ~DialogArray();

    Mode mode() const;

    // Grid layout.
    int columns() const;
    int rows() const;
    qreal spacingX() const;
    qreal spacingY() const;

    // Circular layout.
    int copies() const;        // total instances, original included
    qreal totalAngle() const;  // degrees, counterclockwise; 360 = full circle
    QPointF center() const;
    bool rotateCopies() const;
    // Prefills the center fields - the caller passes the selection's
    // bounding-box center as a sensible starting point.
    void setSuggestedCenter(const QPointF &center);

protected:
    // Captures the canvas click while picking the circular center: left
    // click takes the (object/grid-snapped) point, right click or Escape
    // cancels; mouse moves keep the object-snap highlight live. Installed
    // on the view and its viewport only between startCenterPick() and
    // endCenterPick().
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void syncModeVisibility();
    void startCenterPick();
    void endCenterPick();

    Ui::DialogArray *ui;
    SheetView *m_view;
    bool m_picking = false;
};

#endif // DIALOGARRAY_H
