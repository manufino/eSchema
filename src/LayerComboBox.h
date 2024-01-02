#ifndef LAYERCOMBOBOX_H
#define LAYERCOMBOBOX_H

#include <QComboBox>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QAbstractItemView>


class LayerItemDelegate : public QStyledItemDelegate {
public:
    LayerItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QRect rect = opt.rect;

        // Disegna il rettangolo di sfondo dell'elemento con il colore di sfondo dell'hover
        if (opt.state & QStyle::State_MouseOver) {
            opt.palette.setColor(QPalette::Highlight, QColor(200, 200, 200));
            painter->fillRect(opt.rect, opt.palette.highlight());
        }

        // Disegna il quadratino colorato
        QColor colore = index.data(Qt::UserRole + 1).value<QColor>();
        painter->fillRect(rect.x(), rect.y(), 20, 20, colore);

        // Imposta il colore del testo
        painter->setPen(opt.palette.color(QPalette::WindowText));

        // Disegna il testo a fianco del quadratino
        QString testo = index.data(Qt::DisplayRole).toString();
        QRect textRect = opt.rect.adjusted(25, 0, 0, 0); // Sposta il testo a destra del quadratino
        painter->drawText(textRect, Qt::AlignVCenter, testo);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        // Aggiungi un margine per aumentare la distanza tra le righe della combobox
        QSize hint = QStyledItemDelegate::sizeHint(option, index);
        hint.setHeight(hint.height() + 2);
        return hint;
    }
};

class LayerComboBox : public QComboBox {
    Q_OBJECT
public:
    LayerComboBox(QWidget* parent = nullptr)
        : QComboBox(parent) {
        setItemDelegate(new LayerItemDelegate(this));

        addLayer("Forature", Qt::red);
        addLayer("Strato superiore", Qt::green);
        addLayer("Strato inferiore", Qt::blue);
        addLayer("Strato 1", Qt::darkGreen);
        addLayer("Strato f444a", Qt::lightGray);
        addLayer("Strato ", Qt::cyan);
    }

    void addLayer(const QString& testo, const QColor& colore) {
        QStandardItem* item = new QStandardItem;
        item->setData(colore, Qt::UserRole + 1); // Memorizza le informazioni sul colore in UserRole+1
        item->setData(testo, Qt::DisplayRole);

        // Utilizza insertRow per aggiungere un nuovo elemento alla fine del modello
        QStandardItemModel* standardModel = qobject_cast<QStandardItemModel*>(model());
        if (standardModel)
            standardModel->insertRow(standardModel->rowCount(), item);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        QStyleOptionComboBox opt;
        initStyleOption(&opt);
        opt.rect = QRect(2, 2, width() - 22, 20); // Modificato per spostare il testo più a destra

        // Chiamiamo solo la parte di disegno della combobox che disegna il bordo, la freccia, ecc.
        style()->drawComplexControl(QStyle::CC_ComboBox, &opt, &painter, this);

        // Disegna il quadratino colorato anche sull'elemento selezionato
        if (currentIndex() >= 0 && currentIndex() < count()) {
            QRect colorRect = QRect(5, 4, 20, 16);  // Posizione del quadratino a sinistra dell'elemento selezionato
            QColor colore = itemData(currentIndex(), Qt::UserRole + 1).value<QColor>();
            painter.fillRect(colorRect, colore);

            QRect textRect = opt.rect.adjusted(28, 0, 0, 0);
            painter.setPen(opt.palette.color(QPalette::WindowText));
            painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, currentText());
        }
    }

    QSize sizeHint() const override {
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
};




#endif // LAYERCOMBOBOX_H
