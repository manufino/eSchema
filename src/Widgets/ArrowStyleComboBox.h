/*
 * ArrowStyleComboBox.h
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

#ifndef ARROWSTYLECOMBOBOX_H
#define ARROWSTYLECOMBOBOX_H

#include <QComboBox>
#include <QStyledItemDelegate>

// Graphical picker for the FCJ arrow style, like the reference FidoCadJ
// editor's own parameter dialog: each item paints the actual terminator
// (filled triangle, filled + limiter bar, empty triangle, empty + limiter)
// on a fixed white background with black strokes, whatever the theme -
// the previews mimic the drawing canvas, not the UI chrome. Item order
// follows the FCJ style value itself (bit 0 = limiter, bit 1 = empty).
class ArrowStyleDelegate : public QStyledItemDelegate
{
public:
    explicit ArrowStyleDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

class ArrowStyleComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit ArrowStyleComboBox(QWidget *parent = nullptr);

    // FCJ style value (0..3): bit 0 = limiter bar, bit 1 = empty triangle.
    int currentArrowStyle() const { return m_style; }
    // m_style must be updated here directly, not just via the
    // currentIndexChanged signal: the properties panel calls this under a
    // QSignalBlocker when syncing the display (see PenStyleComboBox's
    // identical setter for the full story).
    void setCurrentArrowStyle(int style);

signals:
    // The user picked a different style (not emitted by
    // setCurrentArrowStyle() under a signal blocker).
    void arrowStyleChanged(int style);

private:
    void paintEvent(QPaintEvent *event) override;

    int m_style = 0;
};

#endif // ARROWSTYLECOMBOBOX_H
