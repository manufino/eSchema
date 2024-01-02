#ifndef LAYER_H
#define LAYER_H

#include <QObject>
#include <QWidget>
#include <QColor>

class Layer
{
public:
    Layer(QString name, QColor color = QColor("black"), int level = 10) {
        m_name = name;
        m_color = color;
        m_level = level;
        m_visible = true;
    }
    // GET
    QColor color() { return m_color; }
    QString name() { return m_name; }
    int level() { return m_level; }
    bool isVisible() { return m_visible; }
    // SET
    void setColor(QColor color) { this->m_color = color; }
    void setVisible(bool visible) { this->m_visible = visible; }
    void setName(QString name) { this->m_name = name; }
    void setLevel(int level) { this->m_level = level; }

    bool operator ==(Layer &layer) const {
        return layer.name() == m_name;
    }

private:
    QColor m_color;
    QString m_name;
    int m_level;
    bool m_visible;
};

#endif // LAYER_H
