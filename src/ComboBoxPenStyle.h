#ifndef COMBOBOXPENSTYLE_H
#define COMBOBOXPENSTYLE_H

#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPen>
#include <QStyledItemDelegate>
#include <QListView>
#include <QEvent>
#include <QCoreApplication>
#include <QHoverEvent>
#include <QPainter>

class PenStyleDelegate : public QStyledItemDelegate
{
public:
    PenStyleDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void initStyleOptionHover(QStyleOptionViewItem &option, const QModelIndex &index) const;
    bool eventFilter(QObject *object, QEvent *event) override;
    void drawItemText(QPainter *painter, const QRect &rect, int alignment,
                              const QPalette &pal, bool enabled, const QString &text,
                              QPalette::ColorRole textRole = QPalette::NoRole) const;

};

/****************************************/
class ComboBoxPenStyle : public QComboBox
{
    Q_OBJECT

public:
    ComboBoxPenStyle(QWidget *parent = nullptr);
    ~ComboBoxPenStyle();

public slots:
    void lineWidthChanged(qreal lineWidth);

private slots:
    void penStyleChanged(int index);

private:
    void paintEvent(QPaintEvent *event) override;
    void setupUi();

    QPen currentPen;
    int lineWidth;
};


#endif // COMBOBOXPENSTYLE_H
