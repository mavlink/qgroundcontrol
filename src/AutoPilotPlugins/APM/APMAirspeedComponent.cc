#include "APMAirspeedComponent.h"

APMAirspeedComponent::APMAirspeedComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
{

}
