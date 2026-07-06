/*
 * LayerLabelName.h
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

#ifndef LAYERLABELNAME_H
#define LAYERLABELNAME_H

#include <QtWidgets>
#include <QObject>
#include "Layer.h"
#include "LayerList.h"

class LayerLabelName : public QWidget {
    Q_OBJECT
public:
    LayerLabelName(Layer *layer, QWidget *parent = nullptr);
    ~LayerLabelName();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void lineEditEditingFinished();

private:
    QLabel *label;
    QLineEdit *lineEdit;
    Layer *layer;
};

#endif // LAYERLABELNAME_H
