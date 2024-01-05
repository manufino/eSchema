#ifndef LABELLAYERNAME_H
#define LABELLAYERNAME_H

#include <QtWidgets>
#include <QObject>

class LabelLayerName : public QWidget {
    Q_OBJECT
public:
    LabelLayerName(const QString &name, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void lineEditEditingFinished();

private:
    QLabel *label;
    QLineEdit *lineEdit;
};

#endif // LABELLAYERNAME_H
