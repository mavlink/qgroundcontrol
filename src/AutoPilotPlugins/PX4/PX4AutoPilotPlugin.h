#pragma once

#include "AutoPilotPlugin.h"
#include "ActuatorComponent.h"
#include "PX4AirframeLoader.h"
#include "AirframeComponent.h"
#include "PX4RadioComponent.h"
#include "ESP8266Component.h"
#include "FlightModesComponent.h"
#include "SensorsComponent.h"
#include "SafetyComponent.h"
#include "PowerComponent.h"
#include "MotorComponent.h"
#include "PX4TuningComponent.h"
#include "PX4FlightBehavior.h"
#include "SyslinkComponent.h"

class Vehicle;

class PX4AutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    PX4AutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    ~PX4AutoPilotPlugin();

    // Overrides from AutoPilotPlugin
    const QVariantList& vehicleComponents(void) override;
    void parametersReadyPreChecks(void) override;
    QString prerequisiteSetup(VehicleComponent* component) const override;

protected:
    bool                    _incorrectParameterVersion; ///< true: parameter version incorrect, setup not allowed
    PX4AirframeLoader*      _airframeFacts;
    AirframeComponent*      _airframeComponent;
    PX4RadioComponent*      _radioComponent;
    ESP8266Component*       _esp8266Component;
    FlightModesComponent*   _flightModesComponent;
    SensorsComponent*       _sensorsComponent;
    SafetyComponent*        _safetyComponent;
    PowerComponent*         _powerComponent;
    MotorComponent*         _motorComponent;
    ActuatorComponent*      _actuatorComponent;
    PX4TuningComponent*     _tuningComponent;
    PX4FlightBehavior*      _flightBehavior;
    SyslinkComponent*       _syslinkComponent;

private:
    QVariantList            _components;
};
