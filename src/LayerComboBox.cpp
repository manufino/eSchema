#include "LayerComboBox.h"

LayerComboBox::LayerComboBox(QWidget* parent)
    : QComboBox(parent)
{
    setItemDelegate(new LayerItemDelegate(this));
    QList<Layer*> *la = LayerList::getInstance().getList();
    addLayerList(la);
    connect(this, &QComboBox::currentIndexChanged,
    this, &LayerComboBox::currentIndexChanged);
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

void LayerComboBox::addLayerList(QList<Layer*> *list)
{
    clear();
    layerList = list;
    for(Layer *layer: *list)
        addLayer(layer->name(), QColor(layer->color()));
}

void LayerComboBox::setMaster(Layer *layer)
{
    int index = -1;
    for (int i = 0; i < layerList->size(); ++i) {
        if (layerList->at(i) == layer) {
            index = i;
            break;
        }
    }
    setCurrentIndex(index);
}

void LayerComboBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.rect = QRect(2, 2, width() - 22, 20);
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
    // TODO: da verificare.
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
    Q_UNUSED(index);

    emit layerSelectedChanged(currentIndex());
}

void LayerComboBox::layerListIsChanged(QList<Layer*> *layerList)
{
    this->layerList = layerList;
}

bool LayerComboBox::layerNameIsUnique(QString name)
{
    for(Layer *layer : *layerList)
        if(layer->name() == name)
            return false;

    return true;
}
