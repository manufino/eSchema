#include "LayerComboBox.h"
#include <QPixmap>
#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QCheckBox>
#include <QApplication>
#include <QFrame>
#include <QMouseEvent>

LayerComboBox::LayerComboBox(QWidget *parent)
    : QWidget(parent), layerActive(true)
{
    QFrame *globFrame = new QFrame(this);

    setStyleSheet("LayerComboBox > QFrame {\
                  border:1px solid gray;\
                  border-top-left-radius:3px; \
                  border-top-right-radius:3px; \
                  height:20px; \
                  width:180px; \
                  }\
                  LayerComboBox > QFrame:hover {\
                  border:1px solid green;\
                  }");

    setCursor(Qt::PointingHandCursor);

    QHBoxLayout *layout = new QHBoxLayout(this);

    layerName = "LAYER 0";
    enabledLabel = new QLabel("", this);

    QPixmap pic(":/res/resources/light-bulb.png");

    enabledLabel->setPixmap(pic.scaled(20, 20, Qt::KeepAspectRatio));
    enabledLabel->setFixedSize(20, 20);

    layout->addWidget(enabledLabel);

    colorLabel = new QLabel(this);
    colorLabel->setFixedSize(20, 20);
    updateColorLabel();
    layout->addWidget(colorLabel);

    QLabel *nameLabel = new QLabel(layerName, this);
    nameLabel->setFont(QFont("Hack", 8));
    layout->addWidget(nameLabel);

    QLabel *downArrowLabel = new QLabel("", this);

    QPixmap pic2(":/res/resources/down.png");

    downArrowLabel->setPixmap(pic2.scaled(10, 10, Qt::KeepAspectRatio));
    downArrowLabel->setFixedSize(10, 10);

    layout->addWidget(downArrowLabel);

    globFrame->setLayout(layout);
}

bool LayerComboBox::isLayerActive() const
{
    return layerActive;
}

QString LayerComboBox::getLayerName() const
{
    return layerName;
}

QColor LayerComboBox::getLayerColor() const
{
    return layerColor;
}

void LayerComboBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    updateColorLabel();
}

void LayerComboBox::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        layerDialog = new LayerDialog(this);
        layerDialog->show();
        //qDebug()<<"LayerComboBox::mousePressEvent(QMouseEvent *event)";
    }
}

void LayerComboBox::handleCheckboxToggled(bool checked)
{
    layerActive = checked;
    emit layerStateChanged(layerName, layerActive);
}

void LayerComboBox::updateColorLabel()
{
    QPixmap pixmap(colorLabel->size());
    pixmap.fill(layerColor);
    colorLabel->setPixmap(pixmap);
}

