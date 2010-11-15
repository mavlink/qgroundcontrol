#ifndef Q3DWIDGETFACTORY_H
#define Q3DWIDGETFACTORY_H

#include <QPointer>

#include "Q3DWidget.h"

class Q3DWidgetFactory
{
public:
    static QPointer<Q3DWidget> get(const std::string& type);
};

#endif // Q3DWIDGETFACTORY_H
