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

bool Parameter::operator ==(const Parameter& other) const
{
    return
            (*simulinkPath) == *(other.simulinkPath)
            && *simulinkName == *(other.simulinkName)
            && componentID == other.componentID
            && *paramID == *(other.paramID)
            && opalID == other.opalID;

}

float Parameter::getValue() //const
{
    unsigned short allocatedParams = 1;
    unsigned short numParams;
//    unsigned short allocatedValues;
    unsigned short numValues = 1;
    unsigned short returnedNumValues;
    double value;

    int returnVal = OpalGetParameters(allocatedParams, &numParams, &opalID,
                                      numValues, &returnedNumValues, &value);

    if (returnVal != EOK)
    {
        return FLT_MAX;
    }

    return static_cast<float>(value);
}
