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
