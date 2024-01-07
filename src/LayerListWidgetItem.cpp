#include "LayerListWidgetItem.h"

LayerListWidgetItem::LayerListWidgetItem(Layer *layer,  QListWidget *parent) : QListWidgetItem(parent) {
    setData(Qt::UserRole+5, QVariant::fromValue(layer));
    setSizeHint(QSize(100, 40));

    widget = new QWidget(parent);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(2, 0, 4, 0);

    ButtonLayerHide *lickableIconLabel = new ButtonLayerHide(layer, widget);
    layout->addWidget(lickableIconLabel);

    ColorPicker *colorPicker = new ColorPicker(widget);
    colorPicker->setColor(layer->color());
    colorPicker->setFixedSize(25, 25);
    layout->addWidget(colorPicker);

    LabelLayerName *label = new LabelLayerName(layer->name(), widget);
    layout->addWidget(label);

    // solo nel layer master aggiungo l'icona
    if (layer->isMaster()) {
        QLabel *nonClickableIconLabel = new QLabel(widget);
        QIcon icon = QIcon(":/res/resources/remix/bookmark-3-line.png");
        QPixmap pixmap = icon.pixmap(icon.actualSize(QSize(28, 28)));
        nonClickableIconLabel->setPixmap(pixmap);
        nonClickableIconLabel->setFixedSize(28, 28);
        layout->addWidget(nonClickableIconLabel);
    }

    widget->setLayout(layout);
}
