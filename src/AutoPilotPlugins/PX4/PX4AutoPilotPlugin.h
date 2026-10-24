#pragma once

#include "ActuatorComponent.h"
#include "AirframeComponent.h"
#include "AutoPilotPlugin.h"
#include "ESP8266Component.h"
#include "FailureInjectionComponent.h"
#include "FlightModesComponent.h"
#include "JoystickComponent.h"
#include "MotorComponent.h"
#include "PX4AirframeLoader.h"
#include "PX4BatteryComponent.h"
#include "PX4CollisionComponent.h"
#include "PX4EscComponent.h"
#include "PX4FailsafesComponent.h"
#include "PX4FlightBehavior.h"
#include "PX4RadioComponent.h"
#include "PX4ReturnLandComponent.h"
#include "PX4SerialPortsComponent.h"
#include "PX4TuningComponent.h"
#include "SensorsComponent.h"
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
    FailureInjectionComponent* _failureInjectionComponent;
    FlightModesComponent*   _flightModesComponent;
    SensorsComponent*       _sensorsComponent;
    PX4ReturnLandComponent* _returnLandComponent;
    PX4CollisionComponent* _collisionComponent;
    PX4FailsafesComponent* _failsafesComponent;
    PX4SerialPortsComponent* _serialPortsComponent;
    PX4EscComponent* _escComponent;
    PX4BatteryComponent* _batteryComponent;
    MotorComponent*         _motorComponent;
    ActuatorComponent*      _actuatorComponent;
    PX4TuningComponent*     _tuningComponent;
    PX4FlightBehavior*      _flightBehavior;
    SyslinkComponent*       _syslinkComponent;
    JoystickComponent*      _joystickComponent;

private:
    QVariantList            _components;
};
