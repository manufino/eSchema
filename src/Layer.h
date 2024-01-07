#ifndef LAYER_H
#define LAYER_H

#include <QObject>
#include <QWidget>
#include <QColor>

class Layer
{
public:

    Layer() = default;
    Layer(QString name, QColor color = QColor("black"),
          bool isMaster=false, int level = 10)
    {
        m_name = name;
        m_color = color;
        m_level = level;
        m_visible = true;
        m_master = isMaster;
    }
    // GET
    QColor color() { return m_color; }
    QString name() { return m_name; }
    int level() { return m_level; }
    bool isVisible() { return m_visible; }
    bool isMaster() { return m_master; }

    // SET
    void setColor(QColor color) { this->m_color = color; }
    void setVisible(bool visible) { this->m_visible = visible; }
    void setName(QString name) { this->m_name = name; }
    void setLevel(int level) { this->m_level = level; }
    void setMaster(bool isMaster) { m_master = isMaster; }

    bool operator ==(Layer &layer) const {
        return layer.name() == m_name;
    }

private:
    QColor m_color;
    QString m_name;
    int m_level;
    bool m_visible;
    bool m_master;
};

#endif // LAYER_H
