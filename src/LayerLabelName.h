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
