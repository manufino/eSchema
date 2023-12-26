#ifndef LAYERCOMBOBOX_H
#define LAYERCOMBOBOX_H

#include <QComboBox>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>
#include <QApplication>
#include <QFrame>


class LayerComboBox : public QWidget
{
    Q_OBJECT

public:
    LayerComboBox(QWidget *parent = nullptr)
        : QWidget(parent), layerActive(true)
    {
        QFrame *globFrame = new QFrame(this);

        setStyleSheet("LayerComboBox > QFrame {\
                      border:1px solid gray;\
                      border-top-left-radius:8px; \
                      border-top-right-radius:8px; \
                      height:22px; \
                      width:180px; }");

        QHBoxLayout *layout = new QHBoxLayout(this);


        layerName = "LAYER PRIMARIO";
        enabledLabel = new  QLabel("", this);

        QPixmap pic(":/res/resources/light-bulb.png");

        enabledLabel->setPixmap(pic.scaled(15,15, Qt::KeepAspectRatio));
        enabledLabel->setMouseTracking(true);
        enabledLabel->setCursor(Qt::PointingHandCursor);
        enabledLabel->setFixedSize(15,15);

        layout->addWidget( enabledLabel);

        colorLabel = new QLabel(this);
        colorLabel->setFixedSize(15, 15);
        updateColorLabel();
        layout->addWidget(colorLabel);

        QLabel *nameLabel = new QLabel(layerName, this);
        nameLabel->setFont(QFont("Hack", 9));
        layout->addWidget(nameLabel);

        QLabel *downArrowLabel = new QLabel("", this);

        QPixmap pic2(":/res/resources/down.png");

        downArrowLabel->setPixmap(pic2.scaled(10,10, Qt::KeepAspectRatio));
        downArrowLabel->setMouseTracking(true);
        downArrowLabel->setCursor(Qt::PointingHandCursor);
        downArrowLabel->setFixedSize(10,10);

        layout->addWidget( downArrowLabel);

        globFrame->setLayout(layout);

        //setLayout(layout);
    }

    bool isLayerActive() const
    {
        return layerActive;
    }

    QString getLayerName() const
    {
        return layerName;
    }

    QColor getLayerColor() const
    {
        return layerColor;
    }

signals:
    void layerStateChanged(const QString &layerName, bool layerActive);

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        updateColorLabel();
    }

private slots:
    void handleCheckboxToggled(bool checked)
    {
        layerActive = checked;
        emit layerStateChanged(layerName, layerActive);
    }

private:
    void updateColorLabel()
    {
        QPixmap pixmap(colorLabel->size());
        pixmap.fill(layerColor);
        colorLabel->setPixmap(pixmap);
    }

    QLabel *enabledLabel;
    QLabel *colorLabel;
    QString layerName;
    QColor layerColor;
    bool layerActive;
};


#endif // LAYERCOMBOBOX_H
