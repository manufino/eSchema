#ifndef COLORSELECTOR_H
#define COLORSELECTOR_H

#include <QLabel>
#include <QColorDialog>
#include <QMouseEvent>

class ColorSelector : public QLabel {
    Q_OBJECT

public:
    ColorSelector(QWidget *parent = nullptr);

    QColor getColor() { return m_color; }
    void setColor(QColor color);

signals:
    void colorChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QColor m_color;

};

#endif // COLORSELECTOR_H
