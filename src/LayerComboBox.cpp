#include "LayerComboBox.h"

LayerComboBox::LayerComboBox(QWidget* parent)
    : QComboBox(parent) {
    setItemDelegate(new LayerItemDelegate(this));

    addLayer("LAYER 0", Qt::black);
    addLayer("Forature", Qt::red);
    addLayer("Strato superiore", Qt::green);
    addLayer("Strato inferiore", Qt::blue);
    addLayer("Strato medio", Qt::darkGreen);
    addLayer("Quote", Qt::lightGray);
    addLayer("MARCATURE ", Qt::cyan);
}

void LayerComboBox::addLayer(const QString& testo, const QColor& colore)
{
    QStandardItem* item = new QStandardItem;
    item->setData(colore, Qt::UserRole + 1);
    item->setData(testo, Qt::DisplayRole);


    QStandardItemModel* standardModel = qobject_cast<QStandardItemModel*>(model());
    if (standardModel)
        standardModel->insertRow(standardModel->rowCount(), item);
}

void LayerComboBox::addLayerList(QList<Layer *> list)
{
    clear();
    layerList = list;

    Layer *layer;
    foreach(layer, list)
        addLayer(layer->name(), QColor(layer->color()));
}

void LayerComboBox::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.rect = QRect(2, 2, width() - 22, 20); // Modificato per spostare il testo più a destra

    // Chiamiamo solo la parte di disegno della combobox che disegna il bordo, la freccia, ecc.
    style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);

    // Disegna il quadratino colorato
    if (currentIndex() >= 0 && currentIndex() < count()) {
        QRect colorRect = QRect(5, 5, 20, 14);
        QColor colore = itemData(currentIndex(), Qt::UserRole + 1).value<QColor>();
        painter.fillRect(colorRect, colore);

        QRect textRect = opt.rect.adjusted(28, 0, 0, 0);
        painter.setPen(opt.palette.color(QPalette::WindowText));
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, currentText());
    }
}

QSize LayerComboBox::sizeHint() const
{
    QSize hint = QComboBox::sizeHint();

    // Trova la larghezza massima del testo nei singoli item
    int maxWidth = 0;
    for (int i = 0; i < count(); ++i) {
        QString text = itemText(i);
        int textWidth = fontMetrics().horizontalAdvance(text);
        maxWidth = qMax(maxWidth, textWidth);
    }
    hint.setWidth(maxWidth + 70);
    return hint;
}

void LayerComboBox::currentIndexChanged(int index)
{
    Layer *layer;

    foreach(layer,layerList)
        if(layer->name() == itemData(currentIndex()))
               emit layerSelectedChanged(layer);
}

void LayerComboBox::layerListIsChanged(QList<Layer *> layerList)
{
    this->layerList = layerList;
}

bool LayerComboBox::layerNameIsUnique(QString name)
{
    Layer *layer;
    foreach(layer, layerList)
        if(layer->name() == name)
            return false;

    return true;
}
