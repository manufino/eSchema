#include "LabelLayerName.h"

LabelLayerName::LabelLayerName(const QString &name, QWidget *parent) : QWidget(parent) {
    // Creazione di QLabel
    label = new QLabel(name, this);
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

    // Impostazione del layout per il widget principale
    setLayout(layout);

    // Connessione dei segnali ai slot
    connect(lineEdit, &QLineEdit::editingFinished, this, &LabelLayerName::lineEditEditingFinished);

}

bool LabelLayerName::eventFilter(QObject *obj, QEvent *event) {
    if (obj == label && event->type() == QEvent::MouseButtonDblClick) {
        // Doppio clic sulla QLabel
        lineEdit->setText(label->text());
        label->setVisible(false);
        lineEdit->setVisible(true);
        lineEdit->setFocus();
        return true;
    }
    return false;
}

void LabelLayerName::lineEditEditingFinished() {
    // Chiamato quando l'editing della QLineEdit è terminato
    label->setText(lineEdit->text());
    label->setVisible(true);
    lineEdit->setVisible(false);
}

