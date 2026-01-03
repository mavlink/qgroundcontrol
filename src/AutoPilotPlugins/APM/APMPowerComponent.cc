#include "APMPowerComponent.h"

APMPowerComponent::APMPowerComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownPowerVehicleComponent, parent)
{

}
