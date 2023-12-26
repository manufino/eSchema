#include "comboboxpenstyle.h"
#include <QPainter>

PenStyleDelegate::PenStyleDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void PenStyleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // Draw a horizontal line with the pen style
    QRect rect = opt.rect;
    int midY = rect.top() + rect.height() / 2;
    QLine line(rect.left() + 10, midY, rect.right() - 10, midY);

    QPen pen(static_cast<Qt::PenStyle>(index.data(Qt::UserRole).toInt()));
    pen.setWidth(1);
    painter->setPen(pen);
    painter->drawLine(line);


}

ComboBoxPenStyle::ComboBoxPenStyle(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

ComboBoxPenStyle::~ComboBoxPenStyle()
{
    // Cleanup, if needed
}

void ComboBoxPenStyle::setupUi()
{
    mainLayout = new QVBoxLayout(this);

    // Create and populate the pen style combobox with custom delegate
    penStyleComboBox = new QComboBox(this);
    penStyleComboBox->setItemDelegate(new PenStyleDelegate(this));

    penStyleComboBox->addItem("Solid Line", QVariant(static_cast<int>(Qt::SolidLine)));
    penStyleComboBox->addItem("Dash Line", QVariant(static_cast<int>(Qt::DashLine)));
    penStyleComboBox->addItem("Dot Line", QVariant(static_cast<int>(Qt::DotLine)));
    penStyleComboBox->addItem("Dash Dot Line", QVariant(static_cast<int>(Qt::DashDotLine)));
    penStyleComboBox->addItem("Dash Dot Dot Line", QVariant(static_cast<int>(Qt::DashDotDotLine)));

    // Connect the signal for style change
    connect(penStyleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(penStyleChanged(int)));

    // Add widgets to the layout
    mainLayout->addWidget(penStyleComboBox);

    // Set initial pen style
    currentPen.setStyle(Qt::SolidLine);

    setLayout(mainLayout);
}

void ComboBoxPenStyle::penStyleChanged(int index)
{
    // Update the pen style based on the selected index
    Qt::PenStyle style = static_cast<Qt::PenStyle>(penStyleComboBox->itemData(index).toInt());
    currentPen.setStyle(style);

    // You can now use 'currentPen' for your drawing operations
    // For example, you can set the pen of a QPainter in your paintEvent:
    // QPainter painter(this);
    // painter.setPen(currentPen);
}