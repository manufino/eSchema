#ifndef GLOBALUTILS_H
#define GLOBALUTILS_H

#include <QDebug>
#include <QRandomGenerator>
#include <QColor>

class Utils {
public:
    static Utils& instance()
    {
        static Utils instance; // Garantisce l'istanza singola
        return instance;
    }

    QColor randColor()
    {
        int red = QRandomGenerator::global()->bounded(256);
        int green = QRandomGenerator::global()->bounded(256);
        int blue = QRandomGenerator::global()->bounded(256);

        return QColor(red, green, blue);
    }

    /**
     * Libera la memoria in modo sicuro.
     */
    template<typename T>
    void DeleteSafely(T*& ptr)
    {
        if(ptr) {
            delete ptr;
            ptr = nullptr;
        } else
            qDebug() << "[DeleteSafely]: Attempting to delete a null pointer.";
    }

private:
    // Costruttore privato per evitare la creazione di istanze esterne
    Utils() {}
    ~Utils() = default;
    Utils(const Utils&) = delete;
    Utils& operator=(const Utils&) = delete;
};

#endif // GLOBALUTILS_H
