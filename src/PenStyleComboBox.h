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

class PenStyleDelegate : public QStyledItemDelegate
{
public:
    PenStyleDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void initStyleOptionHover(QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool eventFilter(QObject *object, QEvent *event) override;
    void drawItemText(QPainter *painter, const QRect &rect, int alignment,
                              const QPalette &pal, bool enabled, const QString &text,
                              QPalette::ColorRole textRole = QPalette::NoRole) const;

};

/****************************************/
class PenStyleComboBox : public QComboBox
{
    Q_OBJECT

public:
    PenStyleComboBox(QWidget *parent = nullptr);
    ~PenStyleComboBox();

public slots:
    void lineWidthChanged(qreal lineWidth);

private slots:
    void penStyleChanged(int index);

private:
    void paintEvent(QPaintEvent *event) override;
    void setupUi();

    QPen currentPen;
    int lineWidth;
};


#endif // PENSTYLECOMBOBOX_H
