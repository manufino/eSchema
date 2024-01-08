#ifndef GLOBALUTILS_H
#define GLOBALUTILS_H
#include <QDebug>
#include <QRandomGenerator>
#include <QColor>

namespace Utils {
    template<typename T>
    void DeleteSafely(T*& ptr) {
        if(ptr) {
            delete ptr;
            ptr = nullptr;
        } else
            qDebug()<<"Tentativo di eliminare un puntatore nullo.";
    }

    QColor randomColor() {
        int red = QRandomGenerator::global()->bounded(256);
        int green = QRandomGenerator::global()->bounded(256);
        int blue = QRandomGenerator::global()->bounded(256);

        return QColor(red, green, blue);
    }
}
#endif // GLOBALUTILS_H
