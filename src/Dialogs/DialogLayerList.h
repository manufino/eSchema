/*
 * DialogLayerList.h
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

#ifndef DIALOGLAYERLIST_H
#define DIALOGLAYERLIST_H

#include <QDialog>

#include "Layer.h"
#include "LayerListView.h"

namespace Ui {
class DialogLayerList;
}

class DialogLayerList : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLayerList(QWidget *parent = nullptr);
    ~DialogLayerList();

public slots:
    void levelUp();
    void levelDown();
    void setAllVisible();
    void setAllHidden();
    void deleteCurrent();
    void addNewLayer();

private:
    bool layerIsSelected();
    QColor randomColor();


private:
    Ui::DialogLayerList *ui;
};

#endif // DIALOGLAYERLIST_H
