#include "PenStyleComboBox.h"

PenStyleDelegate::PenStyleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PenStyleDelegate::initStyleOptionHover(QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(&option, index);

    // Aggiungi il flag Hover
    option.state |= QStyle::State_MouseOver;
}

bool PenStyleDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::HoverEnter ||
            event->type() == QEvent::HoverLeave)
    {
        QHoverEvent *hoverEvent = dynamic_cast<QHoverEvent *>(event);
        if (hoverEvent) {
            QAbstractItemView *view = qobject_cast<QAbstractItemView*>(object);

            if (view) {
                QModelIndex index = view->model()->index(view->currentIndex().row(), 0);
                if (index.isValid()) {
                    // Forza un aggiornamento dell'elemento sotto il cursore
                    QStyleOptionViewItem option;
                    initStyleOptionHover(option, index);
                    // Ottenere la vista associata all'oggetto
                    view->update(index);
                }
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}


void PenStyleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QRect rect = opt.rect;
    int midY = rect.top() + rect.height() / 2;
    QLine line(rect.left() + 10, midY, rect.right() - 10, midY);

    QPen pen(static_cast<Qt::PenStyle>(index.data(Qt::UserRole).toInt()));
    pen.setWidth(2);

    if (opt.state & QStyle::State_MouseOver)
    {
        // Aggiungi l'effetto hover
        //pen.setColor(QColor("white"));
        painter->fillRect(rect, QBrush(QColor("cyan")));
    }

    painter->setPen(pen);
    painter->drawLine(line);

    painter->setPen(QPen(QColor("gray")));
    painter->drawRect(rect);
}


void PenStyleDelegate::drawItemText(QPainter *painter, const QRect &rect, int alignment,
                                    const QPalette &pal, bool enabled, const QString &text,
                                    QPalette::ColorRole textRole) const
{
    Q_UNUSED(painter);
    Q_UNUSED(rect);
    Q_UNUSED(alignment);
    Q_UNUSED(pal);
    Q_UNUSED(enabled);
    Q_UNUSED(textRole);
    Q_UNUSED(text);
}

PenStyleComboBox::PenStyleComboBox(QWidget *parent)
    : QComboBox(parent), lineWidth(1)
{
    setView(new QListView());
    setItemDelegate(new PenStyleDelegate(this));
    setFixedHeight(22);  // Imposta un'altezza fissa per mantenere il disegno visibile
    setupUi();
    setCursor(Qt::PointingHandCursor);

    // Installa l'event filter per gestire gli eventi hover
    view()->installEventFilter(new PenStyleDelegate(this));
}

PenStyleComboBox::~PenStyleComboBox()
{}

void PenStyleComboBox::lineWidthChanged(qreal lineWidth)
{
    this->lineWidth = lineWidth;
    update();
}

void PenStyleComboBox::setupUi()
{
    addItem("", QVariant(static_cast<int>(Qt::SolidLine)));
    addItem("", QVariant(static_cast<int>(Qt::DashLine)));
    addItem("", QVariant(static_cast<int>(Qt::DotLine)));
    addItem("", QVariant(static_cast<int>(Qt::DashDotLine)));
    addItem("", QVariant(static_cast<int>(Qt::DashDotDotLine)));

    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(penStyleChanged(int)));

    currentPen.setStyle(Qt::SolidLine);
}

void PenStyleComboBox::penStyleChanged(int index)
{
    Qt::PenStyle style = static_cast<Qt::PenStyle>(itemData(index).toInt());
    currentPen.setStyle(style);
}

void PenStyleComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // Disattivo il paint del controllo padre,
    // non voglio che venga disegnato il bordo e la freccina
    // delle classiche combobox.

    //QComboBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Disegna la linea selezionata nella ComboBox chiusa
    QRect rect = this->rect();
    int midY = rect.top() + rect.height() / 2;
    QLine line(rect.left()+5, midY, rect.right()-5, midY);

    QPen p = currentPen;
    p.setWidth(lineWidth);
    painter.setPen(p);
    painter.drawLine(line);
    painter.setPen(QPen(QColor("gray"),1));
    painter.drawRoundedRect(rect,3,3);
}
