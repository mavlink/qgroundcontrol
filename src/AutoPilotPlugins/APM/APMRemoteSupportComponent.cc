#include "APMRemoteSupportComponent.h"

APMRemoteSupportComponent::APMRemoteSupportComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}
