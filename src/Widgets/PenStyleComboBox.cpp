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

    QPen pen(static_cast<Qt::PenStyle>(index.data(Qt::UserRole).toInt()));
    pen.setWidth(2);

    if (opt.state & QStyle::State_MouseOver)
    {
        // Add the hover effect
        //pen.setColor(QColor("white"));
        painter->fillRect(rect, QBrush(QColor("cyan")));
    }

    painter->setPen(pen);
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

    // Draw the selected line in the closed ComboBox
    QRect rect = this->rect();
    int midY = rect.top() + rect.height() / 2;
    QLine line(rect.left()+5, midY, rect.right()-5, midY);

    QPen p = currentPen;
    p.setWidth(lineWidth);
    painter.setPen(p);
    painter.drawLine(line);
    painter.setPen(QPen(QColor("gray"),1));
    painter.drawRoundedRect(rect,3,3);
}
