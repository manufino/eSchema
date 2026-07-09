/*
 * LayerLabelName.cpp
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

#include "LayerLabelName.h"

LayerLabelName::LayerLabelName(Layer *layer, QWidget *parent) : QWidget(parent)
{
    this->layer = layer;

    // Create the QLabel
    label = new QLabel(layer->name(), this);
    label->installEventFilter(this);

    // Create the QLineEdit
    lineEdit = new QLineEdit(this);
    lineEdit->setVisible(false);

    // Layout to organize the widgets
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(lineEdit);

    // Horizontal stretch policy
    layout->setStretch(0, 0);  // QLabel doesn't expand
    layout->setStretch(1, 1);  // QLineEdit expands

    setLayout(layout);
    connect(lineEdit, &QLineEdit::editingFinished, this, &LayerLabelName::lineEditEditingFinished);
}

LayerLabelName::~LayerLabelName()
{
    delete lineEdit;
    delete label;
}

bool LayerLabelName::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == label && event->type() == QEvent::MouseButtonDblClick)
    {
        // Double click on the QLabel
        lineEdit->setText(label->text());
        label->setVisible(false);
        lineEdit->setVisible(true);
        lineEdit->setFocus();
        return true;
    }
    return false;
}

void LayerLabelName::lineEditEditingFinished()
{
    // Called when QLineEdit editing has finished
    label->setText(lineEdit->text());
    label->setVisible(true);
    layer->setName(lineEdit->text());
    lineEdit->setVisible(false);
    LayerList::getInstance().update();
}

