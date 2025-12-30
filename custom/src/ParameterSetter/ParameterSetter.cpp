#include "ParameterSetter.h"
#include "Vehicle.h"
#include "ParameterManager.h"
#include "MultiVehicleManager.h"


QString ParameterSetter::getParameter(int compId, QString paramName)
{
    auto vehicle=MultiVehicleManager::instance()->activeVehicle();
    if(vehicle == nullptr){
        return QString();
    }
    auto parameterManager = vehicle->parameterManager();
    auto parameter = parameterManager->getParameter(compId,paramName);
    if(parameter == nullptr){
        return QString();
    }
    return parameter->rawValueString();
}

void ParameterSetter::setParameter(int compId, QString paramName, float value)
{
    auto vehicle=MultiVehicleManager::instance()->activeVehicle();
    if(vehicle == nullptr){
        return;
    }
    auto parameterManager = vehicle->parameterManager();
    auto parameter = parameterManager->getParameter(compId,paramName);
    if(parameter == nullptr){
        return;
    }
   parameter->setRawValue(value);
}
