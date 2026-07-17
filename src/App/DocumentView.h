/*
 * DocumentView.h
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

#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QWidget>

class SheetView;
class RulerWidget;

// The widget one document tab hosts: the same corner + horizontal/vertical
// ruler + SheetView grid the pre-multidocument MainWindow.ui used to build
// as its central widget, now instantiated once per open document so every
// tab has its own view and rulers.
class DocumentView : public QWidget
{
    Q_OBJECT

public:
    explicit DocumentView(QWidget *parent = nullptr);

    SheetView *view() const { return m_view; }
    RulerWidget *horizontalRuler() const { return m_rulerHorizontal; }
    RulerWidget *verticalRuler() const { return m_rulerVertical; }
    QWidget *rulerCorner() const { return m_corner; }

private:
    SheetView *m_view = nullptr;
    RulerWidget *m_rulerHorizontal = nullptr;
    RulerWidget *m_rulerVertical = nullptr;
    QWidget *m_corner = nullptr;
};

#endif // DOCUMENTVIEW_H
