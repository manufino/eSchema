/*
 * LayerLockButton.h
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

#ifndef LAYERLOCKBUTTON_H
#define LAYERLOCKBUTTON_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>

#include "Layer.h"
#include "LayerList.h"

// Mirrors LayerVisibilityButton (same QLabel-based pixmap-swap pattern), but
// for Layer::isLocked(). Unlike the eye button, the master layer CAN'T be
// locked at all here (eSchema's own choice, not FidoCadJ's - see
// LayerList::setLocked()'s guard) - clicking on the master row's lock icon
// is always a no-op.
class LayerLockButton : public QLabel
{
    Q_OBJECT

public:
    explicit LayerLockButton(Layer *layer, QWidget *parent = nullptr);
    void setStatus(bool locked);
    bool getStatus() { return layerIsLocked; }

signals:
    void statusChanged(bool isLocked);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private slots:
    // Same live theme-switch re-tint as LayerVisibilityButton::refreshIcons().
    void refreshIcons();

private:
    QList<QPixmap> images;
    bool layerIsLocked;
    Layer *layer;
};

#endif // LAYERLOCKBUTTON_H
