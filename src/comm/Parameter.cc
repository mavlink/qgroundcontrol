#include "Parameter.h"
using namespace OpalRT;

Parameter::Parameter(char *simulinkPath, char *simulinkName, uint8_t componentID,
                     QGCParamID paramID, unsigned short opalID)
                         : simulinkPath(new QString(simulinkPath)),
                         simulinkName(new QString(simulinkName)),
                         componentID(componentID),
                         paramID(new QGCParamID(paramID)),
                         opalID(opalID)

{
}
Parameter::Parameter(const Parameter &other)
    : componentID(other.componentID),
    opalID(other.opalID)
{
    simulinkPath = new QString(*other.simulinkPath);
    simulinkName = new QString(*other.simulinkName);
    paramID = new QGCParamID(*other.paramID);
}

Parameter::~Parameter()
{
    delete simulinkPath;
    delete simulinkName;
    delete paramID;
}
