#ifndef LAYERCOMBOBOX_H
#define LAYERCOMBOBOX_H

#include <QWidget>

#include "LayerDialog.h"

class QLabel;
class QFrame;
class QHBoxLayout;

class LayerComboBox : public QWidget
{
    Q_OBJECT

public:
    LayerComboBox(QWidget *parent = nullptr);

    bool isLayerActive() const;
    QString getLayerName() const;
    QColor getLayerColor() const;

signals:
    void layerStateChanged(const QString &layerName, bool layerActive);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void handleCheckboxToggled(bool checked);

private:
    void updateColorLabel();

    QLabel *enabledLabel;
    QLabel *colorLabel;
    QString layerName;
    QColor layerColor;
    bool layerActive;
    LayerDialog *layerDialog;
};

#endif // LAYERCOMBOBOX_H
