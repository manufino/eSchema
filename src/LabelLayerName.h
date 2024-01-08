#ifndef LABELLAYERNAME_H
#define LABELLAYERNAME_H

#include <QtWidgets>
#include <QObject>
#include "Layer.h"
#include "LayerList.h"

class LabelLayerName : public QWidget {
    Q_OBJECT
public:
    LabelLayerName(Layer *layer, QWidget *parent = nullptr);
    ~LabelLayerName();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void lineEditEditingFinished();

private:
    QLabel *label;
    QLineEdit *lineEdit;
    Layer *layer;
};

#endif // LABELLAYERNAME_H
