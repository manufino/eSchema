#ifndef GLOBALUTILS_H
#define GLOBALUTILS_H
#include <QDebug>

namespace Utils {
    template<typename T>
    void DeleteSafely(T*& ptr) {
        if(ptr) {
            delete ptr;
            ptr = nullptr;
        } else
            qDebug()<<"Tentativo di eliminare un puntatore nullo.";
    }
}
#endif // GLOBALUTILS_H
