#include "APMFlightModesComponent.h"

APMFlightModesComponent::APMFlightModesComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownFlightModesVehicleComponent, parent)
{

}
