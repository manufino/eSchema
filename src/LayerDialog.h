#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include <QDialog>

namespace Ui {
class LayerDialog;
}

class LayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerDialog(QWidget *parent = nullptr);
    ~LayerDialog();

private:
    Ui::LayerDialog *ui;
};

#endif // LAYERDIALOG_H
