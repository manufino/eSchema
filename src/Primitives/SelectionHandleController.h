/*
 * SelectionHandleController.h
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

#ifndef SELECTIONHANDLECONTROLLER_H
#define SELECTIONHANDLECONTROLLER_H

#include <QObject>
#include <QList>

class Sheet;
class PrimitiveHandleItem;

// Shows resize-grip handles for the currently selected primitive. Scoped to a
// single-primitive selection - resizing multiple primitives at once has no
// single sensible meaning (each primitive type has a different control-point
// count/role), so a multi-selection simply shows no handles.
class SelectionHandleController : public QObject
{
    Q_OBJECT
public:
    explicit SelectionHandleController(Sheet *sheet, QObject *parent = nullptr);

private slots:
    void onSelectionChanged();
    // Re-syncs handle positions from their primitive's live control points.
    // Needed because dragging the primitive body itself (not a handle) also
    // moves its control points (see GraphicsPrimitive::itemChange), which the
    // handles must follow.
    void refreshHandlePositions();

private:
    void clearHandles();

    Sheet *m_sheet;
    QList<PrimitiveHandleItem *> m_handles;
};

#endif // SELECTIONHANDLECONTROLLER_H
