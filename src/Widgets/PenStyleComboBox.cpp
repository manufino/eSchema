/*
 * PenStyleComboBox.cpp
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

#include "PenStyleComboBox.h"
#include "GraphicsPrimitive.h"

namespace {

// The preview pen for one style: black on the fixed white background, with
// the very same FidoCadJ dash patterns the canvas draws
// (GraphicsPrimitive::fidoDashPattern()), rendered at a small "zoom" so
// the pitch differences stay legible at widget size.
QPen previewPen(Qt::PenStyle style)
{
    QPen pen(Qt::black);
    pen.setWidthF(2.0);
    pen.setCapStyle(Qt::RoundCap);
    const QVector<qreal> pattern = GraphicsPrimitive::fidoDashPattern(style);
    if (pattern.isEmpty())
        return pen;
    constexpr qreal PreviewZoom = 2.0;
    QList<qreal> widthUnits;
    widthUnits.reserve(pattern.size());
    for (qreal length : pattern)
        widthUnits.append(length * PreviewZoom / pen.widthF());
    pen.setDashPattern(widthUnits);
    return pen;
}

}

PenStyleDelegate::PenStyleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PenStyleDelegate::initStyleOptionHover(QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(&option, index);

    // Add the Hover flag
    option.state |= QStyle::State_MouseOver;
}

bool PenStyleDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::HoverEnter ||
            event->type() == QEvent::HoverLeave)
    {
        QHoverEvent *hoverEvent = dynamic_cast<QHoverEvent *>(event);
        if (hoverEvent) {
            QAbstractItemView *view = qobject_cast<QAbstractItemView*>(object);

            if (view) {
                QModelIndex index = view->model()->index(view->currentIndex().row(), 0);
                if (index.isValid()) {
                    // Force an update of the item under the cursor
                    QStyleOptionViewItem option;
                    initStyleOptionHover(option, index);
                    // Get the view associated with the object
                    view->update(index);
                }
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}


void PenStyleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QRect rect = opt.rect;
    int midY = rect.top() + rect.height() / 2;
    QLine line(rect.left() + 10, midY, rect.right() - 10, midY);

    // Always white background with a black line, whatever the theme - the
    // previews mimic the drawing canvas, not the UI chrome.
    painter->fillRect(rect, opt.state & QStyle::State_MouseOver
                      ? QColor(0xd8, 0xe6, 0xf5) : QColor(Qt::white));

    painter->setPen(previewPen(static_cast<Qt::PenStyle>(index.data(Qt::UserRole).toInt())));
    painter->drawLine(line);

    painter->setPen(QPen(QColor("gray")));
    painter->drawRect(rect);
}


void PenStyleDelegate::drawItemText(QPainter *painter, const QRect &rect, int alignment,
                                    const QPalette &pal, bool enabled, const QString &text,
                                    QPalette::ColorRole textRole) const
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
    Q_UNUSED(alignment);
    Q_UNUSED(pal);
    Q_UNUSED(enabled);
    Q_UNUSED(textRole);
    Q_UNUSED(text);
}

PenStyleComboBox::PenStyleComboBox(QWidget *parent)
    : QComboBox(parent), lineWidth(1)
{
    setView(new QListView());
    setItemDelegate(new PenStyleDelegate(this));
    setFixedHeight(22);  // Set a fixed height to keep the drawing visible
    setupUi();
    setCursor(Qt::PointingHandCursor);

    // Install the event filter to handle hover events
    view()->installEventFilter(new PenStyleDelegate(this));
}

PenStyleComboBox::~PenStyleComboBox()
{}

void PenStyleComboBox::lineWidthChanged(qreal lineWidth)
{
    this->lineWidth = lineWidth;
    update();
}

void PenStyleComboBox::setupUi()
{
    addItem("", QVariant(static_cast<int>(Qt::SolidLine)));
    addItem("", QVariant(static_cast<int>(Qt::DashLine)));
    addItem("", QVariant(static_cast<int>(Qt::DotLine)));
    addItem("", QVariant(static_cast<int>(Qt::DashDotLine)));
    addItem("", QVariant(static_cast<int>(Qt::DashDotDotLine)));

    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(penStyleChanged(int)));

    currentPen.setStyle(Qt::SolidLine);
}

void PenStyleComboBox::penStyleChanged(int index)
{
    Qt::PenStyle style = static_cast<Qt::PenStyle>(itemData(index).toInt());
    currentPen.setStyle(style);
}

void PenStyleComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // Skip the parent control's paint - the border and little arrow of a
    // regular combobox aren't wanted here.

    //QComboBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw the selected line in the closed ComboBox - white canvas-like
    // background and black line whatever the theme, same FidoCadJ dash
    // pattern as the drawing itself.
    QRect rect = this->rect();
    int midY = rect.top() + rect.height() / 2;
    QLine line(rect.left()+5, midY, rect.right()-5, midY);

    painter.setPen(QPen(QColor("gray"), 1));
    painter.setBrush(QColor(Qt::white));
    painter.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 3, 3);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(previewPen(currentPen.style()));
    painter.drawLine(line);
}
