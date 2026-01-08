#include "APMAirframeComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"

APMAirframeComponent::APMAirframeComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{
    ParameterManager *const paramMgr = vehicle->parameterManager();

    if (paramMgr->parameterExists(ParameterManager::defaultComponentId, _frameClassParam)) {
        _frameClassFact = paramMgr->getParameter(ParameterManager::defaultComponentId, _frameClassParam);
        if (vehicle->vehicleType() != MAV_TYPE_HELICOPTER) {
            _requiresFrameSetup = true;
        }
    } else {
        _frameClassFact = nullptr;
    }
}

QStringList APMAirframeComponent::setupCompleteChangedTriggerList() const
{
    QStringList list;

    if (_requiresFrameSetup) {
        list << _frameClassParam;
    }

    return list;
}

bool APMAirframeComponent::setupComplete() const
{
    return (_requiresFrameSetup ? (_frameClassFact->rawValue().toInt() != 0) : true);
}
