/*
 * PenStyleComboBox.h
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

#ifndef PENSTYLECOMBOBOX_H
#define PENSTYLECOMBOBOX_H

#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPen>
#include <QStyledItemDelegate>
#include <QListView>
#include <QEvent>
#include <QCoreApplication>
#include <QHoverEvent>
#include <QPainter>

// Paints one row of PenStyleComboBox's popup: a sample line drawn with the
// row's actual pen style (FidoCadJ dash pattern) in black on a fixed white
// background, whatever the theme - the preview mimics the drawing canvas.
class PenStyleDelegate : public QStyledItemDelegate
{
public:
    PenStyleDelegate(QObject *parent = nullptr);

    // White background, hover frame, then the sample line for the row's style.
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    // Fills the option's hover state (tracked by the eventFilter below).
    void initStyleOptionHover(QStyleOptionViewItem &option, const QModelIndex &index) const;
    // Tracks hover movement over the popup list to repaint the hover frame.
    bool eventFilter(QObject *object, QEvent *event) override;
    void drawItemText(QPainter *painter, const QRect &rect, int alignment,
                              const QPalette &pal, bool enabled, const QString &text,
                              QPalette::ColorRole textRole = QPalette::NoRole) const;

};

/****************************************/
// Line-style picker (Properties panel): each entry previews one of the 5
// FidoCadJ dash styles with its real pattern (GraphicsPrimitive::
// fidoDashPattern()), both in the popup and on the closed combo itself.
class PenStyleComboBox : public QComboBox
{
    Q_OBJECT

public:
    PenStyleComboBox(QWidget *parent = nullptr);
    ~PenStyleComboBox();

    // The style currently shown/selected.
    Qt::PenStyle currentPenStyle() const { return currentPen.style(); }
    // Selects the item matching `style`. currentPen must be updated here
    // directly, not just via the currentIndexChanged->penStyleChanged
    // signal: the properties panel calls this under a QSignalBlocker (to
    // sync the display without re-applying the edit), which would leave
    // the closed combo painting the previous style - a dashed primitive
    // read back as "solid".
    void setCurrentPenStyle(Qt::PenStyle style)
    {
        currentPen.setStyle(style);
        setCurrentIndex(findData(QVariant(static_cast<int>(style))));
        update();
    }

public slots:
    // Thickens the preview strokes to match the document line width.
    void lineWidthChanged(qreal lineWidth);

private slots:
    // The user picked a row: updates currentPen and re-emits the change to
    // whoever is connected (the properties panel applies it undoably).
    void penStyleChanged(int index);

private:
    // Draws the closed combo as a white swatch with the current style's
    // sample line (same look as the popup rows).
    void paintEvent(QPaintEvent *event) override;
    // Populates the 5 style items and installs the delegate.
    void setupUi();

    QPen currentPen;
    int lineWidth;
};


#endif // PENSTYLECOMBOBOX_H
