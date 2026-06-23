#include "APMESCComponent.h"

APMESCComponent::APMESCComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownESCVehicleComponent, parent)
{

}
