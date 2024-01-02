#ifndef LAYERTOOLBARWIDGET_H
#define LAYERTOOLBARWIDGET_H

#include <QWidget>

namespace Ui {
class LayerToolBarWidget;
}

class LayerToolBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LayerToolBarWidget(QWidget *parent = nullptr);
    ~LayerToolBarWidget();

private:
    Ui::LayerToolBarWidget *ui;
};

#endif // LAYERTOOLBARWIDGET_H
