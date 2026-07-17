/*
 * DocumentView.cpp
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

#include "DocumentView.h"
#include "SheetView.h"
#include "RulerWidget.h"
#include <QGridLayout>

DocumentView::DocumentView(QWidget *parent)
    : QWidget(parent)
{
    // Same layout, sizes and view settings the pre-multidocument
    // MainWindow.ui declared for its central widget.
    auto *layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    m_corner = new QWidget(this);
    m_corner->setMinimumSize(14, 14);
    m_corner->setMaximumSize(14, 14);
    layout->addWidget(m_corner, 0, 0);

    m_rulerHorizontal = new RulerWidget(this);
    layout->addWidget(m_rulerHorizontal, 0, 1);

    m_rulerVertical = new RulerWidget(this);
    m_rulerVertical->setOrientation(Qt::Vertical);
    layout->addWidget(m_rulerVertical, 1, 0);

    m_view = new SheetView(this);
    // Keep this small: the canvas scrolls anyway, and anything larger
    // inflates the whole window's minimum height/width.
    m_view->setMinimumSize(200, 150);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    layout->addWidget(m_view, 1, 1);
}
