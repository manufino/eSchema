/*
 * GridPreviewWidget.h
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

#ifndef GRIDPREVIEWWIDGET_H
#define GRIDPREVIEWWIDGET_H

#include <QWidget>
#include <QColor>

// Live preview for the Options dialog's Grid page: repaints a small swatch
// of the grid exactly as SheetView::drawBackground() would draw it, from
// the values currently sitting in the page's own controls - so the user
// sees the effect of every change before hitting Apply.
class GridPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GridPreviewWidget(QWidget *parent = nullptr);

    // Same 0/1/2 encoding as the Grid page's type combo and the "grid_type"
    // setting: 0 = lines+dots, 1 = dots, 2 = lines.
    void setGridType(int type) { m_type = type; update(); }
    // The remaining setters mirror the Grid page's controls one-to-one;
    // each one just stores the value and repaints the swatch.
    void setStep(int step) { m_step = qMax(1, step); update(); }
    void setMarkStep(int markStep) { m_markStep = qMax(1, markStep); update(); }
    void setDotColor(const QColor &color) { m_dotColor = color; update(); }
    void setLineColor(const QColor &color) { m_lineColor = color; update(); }
    void setMarkColor(const QColor &color) { m_markColor = color; update(); }
    void setBackgroundColor(const QColor &color) { m_backgroundColor = color; update(); }
    void setLineWidth(qreal width) { m_lineWidth = width; update(); }
    void setMarkWidth(qreal width) { m_markWidth = width; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_type = 0;
    int m_step = 10;
    int m_markStep = 5;
    QColor m_dotColor = Qt::gray;
    QColor m_lineColor = Qt::lightGray;
    QColor m_markColor = Qt::darkGray;
    QColor m_backgroundColor = Qt::white;
    qreal m_lineWidth = 0.5;
    qreal m_markWidth = 1.0;
};

#endif // GRIDPREVIEWWIDGET_H
