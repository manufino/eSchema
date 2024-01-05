#include "LayerItemDelegate.h"

LayerItemDelegate::LayerItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void LayerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
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
    QRect colorRect = QRect(rect.x(), rect.y(), 20, 20);
    painter->fillRect(colorRect, colore);

    // Imposta il colore del testo
    painter->setPen(opt.palette.color(QPalette::WindowText));

    // Disegna il testo a fianco del quadratino
    QString testo = index.data(Qt::DisplayRole).toString();
    QRect textRect = opt.rect.adjusted(25, 0, 0, 0); // Sposta il testo a destra del quadratino
    painter->drawText(textRect, Qt::AlignVCenter, testo);
}

QSize LayerItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    // Aggiungi un margine per aumentare la distanza tra le righe della combobox
    QSize hint = QStyledItemDelegate::sizeHint(option, index);
    hint.setHeight(hint.height() + 2);
    hint.setWidth(hint.width() - 40);
    return hint;
}
