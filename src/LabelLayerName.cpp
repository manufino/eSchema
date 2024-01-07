#include "LabelLayerName.h"

LabelLayerName::LabelLayerName(Layer *layer, QWidget *parent) : QWidget(parent)
{
    this->layer = layer;

    // Creazione di QLabel
    label = new QLabel(layer->name(), this);
    label->installEventFilter(this);

    // Creazione di QLineEdit
    lineEdit = new QLineEdit(this);
    lineEdit->setVisible(false);

    // Layout per organizzare i widget
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(lineEdit);

    // Impostazione della politica di stretching orizzontale
    layout->setStretch(0, 0);  // QLabel non si espande
    layout->setStretch(1, 1);  // QLineEdit si espande

    setLayout(layout);
    connect(lineEdit, &QLineEdit::editingFinished, this, &LabelLayerName::lineEditEditingFinished);
}

bool LabelLayerName::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == label && event->type() == QEvent::MouseButtonDblClick)
    {
        // Doppio clic sulla QLabel
        lineEdit->setText(label->text());
        label->setVisible(false);
        lineEdit->setVisible(true);
        lineEdit->setFocus();
        return true;
    }
    return false;
}

void LabelLayerName::lineEditEditingFinished()
{
    // Chiamato quando l'editing della QLineEdit è terminato
    label->setText(lineEdit->text());
    label->setVisible(true);
    layer->setName(lineEdit->text());
    lineEdit->setVisible(false);
    LayerList::getInstance().update();
}

