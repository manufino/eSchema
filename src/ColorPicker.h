#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QLabel>
#include <QColorDialog>
#include <QMouseEvent>

class ColorPicker : public QLabel {
    Q_OBJECT

public:
    ColorPicker(QWidget *parent = nullptr);

    QColor getColor() { return m_color; }
    void setColor(QColor color);

signals:
    void colorChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QColor m_color;

};

#endif // COLORPICKER_H
