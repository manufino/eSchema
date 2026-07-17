/*
 * ArrowStyleComboBox.cpp
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

#include "ArrowStyleComboBox.h"
#include "PrimitiveArrowUtils.h"
#include <QListView>
#include <QPainter>

namespace {

// FidoCadJ arrowStyle bits (Arrow.java).
constexpr int FlagLimiter = 0x01;
constexpr int FlagEmpty = 0x02;

// One terminator preview: a short line ending in the style's arrowhead,
// black on the white canvas-like background.
void paintPreview(QPainter *painter, const QRect &rect, int style)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::black);
    pen.setWidthF(1.4);
    painter->setPen(pen);

    const int midY = rect.top() + rect.height() / 2;
    const QPointF tail(rect.left() + 8, midY);
    const QPointF tip(rect.right() - 10, midY);
    const QPointF base = PrimitiveArrowUtils::paintArrow(
                painter, tip, tail, style & FlagLimiter, style & FlagEmpty,
                /*length=*/9.0, /*halfWidth=*/4.0);
    painter->drawLine(tail, base);
    painter->restore();
}

}

ArrowStyleDelegate::ArrowStyleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ArrowStyleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    painter->fillRect(opt.rect, opt.state & QStyle::State_MouseOver
                      ? QColor(0xd8, 0xe6, 0xf5) : QColor(Qt::white));
    paintPreview(painter, opt.rect, index.data(Qt::UserRole).toInt());
    painter->setPen(QPen(QColor("gray")));
    painter->drawRect(opt.rect);
}

ArrowStyleComboBox::ArrowStyleComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setView(new QListView());
    setItemDelegate(new ArrowStyleDelegate(this));
    setFixedHeight(22);
    setCursor(Qt::PointingHandCursor);

    for (int style = 0; style <= 3; ++style)
        addItem(QString(), QVariant(style));

    connect(this, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_style = itemData(index).toInt();
        emit arrowStyleChanged(m_style);
        update();
    });
}

void ArrowStyleComboBox::setCurrentArrowStyle(int style)
{
    m_style = style;
    setCurrentIndex(findData(QVariant(style)));
    update();
}

void ArrowStyleComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // White canvas-like background and black terminator whatever the
    // theme, same as the dropdown items.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRect rect = this->rect();
    painter.setPen(QPen(QColor("gray"), 1));
    painter.setBrush(QColor(Qt::white));
    painter.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 3, 3);
    painter.setBrush(Qt::NoBrush);
    paintPreview(&painter, rect, m_style);
}
