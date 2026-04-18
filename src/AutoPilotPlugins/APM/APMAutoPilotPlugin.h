#pragma once

#include "AutoPilotPlugin.h"

class APMAirframeComponent;
class APMAirspeedComponent;
class APMFlightModesComponent;
class APMRadioComponent;
class APMTuningComponent;
class APMAdvancedTuningCopterComponent;
class APMFailsafesComponent;
class APMFlightSafetyComponent;
class APMSensorsComponent;
class APMESCComponent;
class APMPowerComponent;
class APMMotorComponent;
class APMGimbalComponent;
class APMLightsComponent;
class APMSubFrameComponent;
class APMServoComponent;
class ESP8266Component;
class APMHeliComponent;
class APMRemoteSupportComponent;
class APMFollowComponent;
class JoystickComponent;
class ScriptingComponent;
class Vehicle;

/// This is the AutoPilotPlugin implementation for the MAV_AUTOPILOT_ARDUPILOT type.
class APMAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    explicit APMAutoPilotPlugin(Vehicle *vehicle, QObject *parent = nullptr);
    ~APMAutoPilotPlugin();

    const QVariantList &vehicleComponents() override;
    QString prerequisiteSetup(VehicleComponent *component) const override;

protected:
    bool _incorrectParameterVersion = false; ///< true: parameter version incorrect, setup not allowed
    APMAirframeComponent *_airframeComponent = nullptr;
    APMAirspeedComponent *_airspeedComponent = nullptr;
    APMGimbalComponent *_gimbalComponent = nullptr;
    APMLightsComponent *_lightsComponent = nullptr;
    APMSubFrameComponent *_subFrameComponent = nullptr;
    APMFlightModesComponent *_flightModesComponent = nullptr;
    APMServoComponent *_servoComponent = nullptr;
    APMPowerComponent *_powerComponent = nullptr;
    APMESCComponent *_escComponent = nullptr;
    APMMotorComponent *_motorComponent = nullptr;
    APMRadioComponent *_radioComponent = nullptr;
    APMFailsafesComponent *_failsafesComponent = nullptr;
    APMFlightSafetyComponent *_flightSafetyComponent = nullptr;
    APMSensorsComponent *_sensorsComponent = nullptr;
    APMTuningComponent *_tuningComponent = nullptr;
    APMAdvancedTuningCopterComponent *_advancedTuningCopterComponent = nullptr;
    ESP8266Component *_esp8266Component = nullptr;
    APMHeliComponent *_heliComponent = nullptr;
    APMRemoteSupportComponent *_apmRemoteSupportComponent = nullptr;
    APMFollowComponent *_followComponent = nullptr;
    JoystickComponent *_joystickComponent = nullptr;
    ScriptingComponent *_scriptingComponent = nullptr;

#ifndef QGC_NO_SERIAL_LINK
private slots:
    /// Executed when the Vehicle is parameter ready. It checks for the service bulletin against Cube Blacks.
    void _checkForBadCubeBlack(bool parametersReady);
#endif

private:
    QVariantList _components;
};
