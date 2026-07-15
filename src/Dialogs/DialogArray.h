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

// Parameters for Edit > Array of copies. Two layouts: a columns x rows grid
// with configurable steps, or a circular (polar) series of instances around
// a center - AutoCAD-style: `copies()` total instances spread over
// `totalAngle()` degrees, each optionally rotated to follow the circle.
//
// The circular center can be typed in, or picked directly on the canvas:
// "Pick from canvas" finishes exec() with the PickRequested result code -
// the *caller* then runs the actual canvas pick (with no modal window left
// to block input), stores the point via setSuggestedCenter(), and exec()s
// this same instance again, every other field preserved. A hidden-but-
// still-exec()ing modal dialog proved unreliable at releasing its input
// block, hence this close-pick-reopen round trip instead.
class DialogArray : public QDialog
{
    Q_OBJECT

public:
    enum class Mode { Grid, Circular };

    // exec() result code (alongside Accepted/Rejected) meaning "the user
    // asked to pick the circular center on the canvas".
    static constexpr int PickRequested = 100;

    explicit DialogArray(QWidget *parent = nullptr);
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

private:
    void syncModeVisibility();

    Ui::DialogArray *ui;
};

#endif // DIALOGARRAY_H
