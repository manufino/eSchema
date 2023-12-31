#ifndef COMBOBOXPENSTYLE_H
#define COMBOBOXPENSTYLE_H

#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPen>
#include <QStyledItemDelegate>


class PenStyleDelegate : public QStyledItemDelegate
{
public:
    PenStyleDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};


class ComboBoxPenStyle : public QWidget
{
    Q_OBJECT

public:
    ComboBoxPenStyle(QWidget *parent = nullptr);
    ~ComboBoxPenStyle();

private slots:
    void penStyleChanged(int index);

private:
    void setupUi();

    QVBoxLayout *mainLayout;
    QComboBox *penStyleComboBox;
    QPen currentPen;
};


#endif // COMBOBOXPENSTYLE_H
