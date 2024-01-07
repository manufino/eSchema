#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include <QDialog>
#include <QRandomGenerator>

#include "Layer.h"
#include "LayerListView.h"

namespace Ui {
class LayerDialog;
}

class LayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerDialog(QWidget *parent = nullptr);
    ~LayerDialog();

public slots:
    void levelUp();
    void levelDown();
    void setAllVisible();
    void setAllHidden();
    void deleteCurrent();
    void addNewLayer();

private:
    bool layerIsSelected();


private:
    Ui::LayerDialog *ui;
};

#endif // LAYERDIALOG_H
