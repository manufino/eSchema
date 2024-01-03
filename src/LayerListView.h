#ifndef LAYERLISTVIEW_H
#define LAYERLISTVIEW_H


#include <QListWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>

#include "Layer.h"
#include "ColorPicker.h"

class LayerListView : public QListWidget {
    Q_OBJECT
public:
    LayerListView(QWidget *parent = nullptr) : QListWidget(parent) {}

    void addLayer(Layer *layer) {
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setSizeHint(QSize(100, 40));

        QWidget *widget = new QWidget(this);
        QHBoxLayout *layout = new QHBoxLayout(widget);

        // Clickable Icon
        QPushButton *clickableIconButton = new QPushButton(widget);
        clickableIconButton->setIcon(QIcon(":/res/resources/light-bulb.png"));
        clickableIconButton->setFlat(true);
        clickableIconButton->setCheckable(true);
        layout->addWidget(clickableIconButton);

        // Color Picker
        ColorPicker *colorPicker = new ColorPicker(widget);
        colorPicker->setColor(layer->color());

        layout->addWidget(colorPicker);

        // Text
        QLabel *label = new QLabel(layer->name(), widget);
        layout->addWidget(label);

        // Non-clickable Icon
        QLabel *nonClickableIconLabel = new QLabel(widget);
        QIcon icon = QIcon(":/res/resources/light-bulb.png");
        QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(32, 32)));
        nonClickableIconLabel->setPixmap(pixmap);
        layout->addWidget(nonClickableIconLabel);

        widget->setLayout(layout);
        setItemWidget(item, widget);
    }

signals:
    void colorChanged(QListWidgetItem *item);

private slots:
    void handleColorPickerClick(QListWidgetItem *item) {
        QColorDialog colorDialog;
        QColor selectedColor = colorDialog.getColor();
        if (selectedColor.isValid()) {
            emit colorChanged(item);
        }
    }
};


#endif // LAYERLISTVIEW_H
