#include "ESP8266Component.h"

ESP8266Component::ESP8266Component(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("WiFi Bridge"))
{

}
