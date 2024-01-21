#ifndef OBJECTPROPERTIES_H
#define OBJECTPROPERTIES_H

#include <QObject>
#include <QWidget>
#include "Layer.h"

class ObjectProperties : public QObject
{
    Q_OBJECT
public:
    ObjectProperties(QObject *parent = nullptr);
    ~ObjectProperties();

private:



};

#endif // OBJECTPROPERTIES_H
