#ifndef PARAMETER_H
#define PARAMETER_H

#include <QString>
#include "mavlink_types.h"
#include "QGCParamID.h"

namespace OpalRT
{
class Parameter
{
public:
    Parameter();

protected:
    QString *simulinkName;
    QString *simulinkPath;
    unsigned short opalID;

    QGCParamID *paramID;
    uint8_t componentID;
};
}

#endif // PARAMETER_H
