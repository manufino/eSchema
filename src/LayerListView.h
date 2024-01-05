#ifndef LAYERLISTVIEW_H
#define LAYERLISTVIEW_H


#include <QListWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QLineEdit>
#include "Layer.h"
#include "ColorPicker.h"
#include "LayerList.h"


class LayerListView : public QListWidget {
    Q_OBJECT
public:
    LayerListView(QWidget *parent = nullptr) : QListWidget(parent) {
        addLayerList(LayerList::getInstance().getList());
/*
        connect(&LayerList::getInstance(),
        &LayerList::layerListChanged, this, &LayerListView::addLayerList);*/
    }



    void addLayer(Layer *layer) {
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setSizeHint(QSize(100, 40));

        QWidget *widget = new QWidget(this);
        QHBoxLayout *layout = new QHBoxLayout(widget);

        layout->setContentsMargins(2,0,4,0);

        // Clickable Icon
        QLabel *lickableIconLabel = new QLabel(widget);
        QIcon icon2 = QIcon(":/res/resources/remix/eye-line.png");
        QPixmap pixmap2 = icon2.pixmap(icon2.actualSize(QSize(28, 28)));
        lickableIconLabel->setPixmap(pixmap2);
        lickableIconLabel->setCursor(Qt::PointingHandCursor);
        lickableIconLabel->setFixedSize(28,28);

        layout->addWidget(lickableIconLabel);


        // Color Picker
        ColorPicker *colorPicker = new ColorPicker(widget);
        colorPicker->setColor(layer->color());
        colorPicker->setFixedSize(25,25);
        layout->addWidget(colorPicker);

        // Text
        QLineEdit *label = new QLineEdit(widget);
        label->setText(layer->name());

        layout->addWidget(label);

        if(layer->isMaster())
        {
            QLabel *nonClickableIconLabel = new QLabel(widget);
            QIcon icon = QIcon(":/res/resources/remix/bookmark-3-line.png");
            QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(28, 28)));
            nonClickableIconLabel->setPixmap(pixmap);
            nonClickableIconLabel->setFixedSize(28,28);
            layout->addWidget(nonClickableIconLabel);
        }
        widget->setLayout(layout);
        setItemWidget(item, widget);
    }

signals:


public slots:
    void addLayerList(QList<Layer> *layerList) {
        this->clear();
        for (Layer layer : *layerList) {
            addLayer(&layer);
        }
    }
};


#endif // LAYERLISTVIEW_H
