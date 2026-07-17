/*
 * DockTitleTab.h
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

#ifndef DOCKTITLETAB_H
#define DOCKTITLETAB_H

#include <QWidget>

class QDockWidget;

// A slim, tab-looking replacement for a QDockWidget's native title bar,
// used whenever a dock still needs a drag handle but the fat native bar is
// unwanted: a floating dock, or a lone split pane sitting next to other
// docks. Tabbed docks don't use it at all (their tab is the handle).
//
// Mouse presses/moves outside the close glyph are deliberately ignored so
// they propagate to the QDockWidget, which keeps Qt's native dock dragging
// (splits, tabbing, floating - with drop indicators) fully working; only
// the X consumes its click and emits closeRequested().
class DockTitleTab : public QWidget
{
    Q_OBJECT

public:
    explicit DockTitleTab(QDockWidget *dock);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    // Repaints when the dock's windowTitle changes (the title is read live
    // at paint time).
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QRect closeRect() const;

    QDockWidget *m_dock;
    bool m_hoverClose = false;
    bool m_pressedClose = false;
};

#endif // DOCKTITLETAB_H
