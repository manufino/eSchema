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

// The editable layer-name cell of a layer manager row: a QLabel showing the
// name that swaps into a QLineEdit on double click; confirming the edit
// renames the Layer and broadcasts it via LayerList::update().
class LayerLabelName : public QWidget {
    Q_OBJECT
public:
    // `layer` is borrowed (owned by LayerList) and must outlive this widget.
    LayerLabelName(Layer *layer, QWidget *parent = nullptr);
    ~LayerLabelName();

protected:
    // Catches the label's double click to start inline editing (label
    // hidden, line edit shown and focused).
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // Commits the edit: updates the label, renames the Layer, swaps the
    // line edit away again, and fires LayerList::update().
    void lineEditEditingFinished();

private:
    QLabel *label;
    QLineEdit *lineEdit;
    Layer *layer;
};

#endif // LAYERLABELNAME_H
