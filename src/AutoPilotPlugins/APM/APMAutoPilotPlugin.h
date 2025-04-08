/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include "AutoPilotPlugin.h"

class APMAirframeComponent;
class APMFlightModesComponent;
class APMRadioComponent;
class APMTuningComponent;
class APMSafetyComponent;
class APMSensorsComponent;
class APMPowerComponent;
class APMMotorComponent;
class APMCameraComponent;
class APMLightsComponent;
class APMSubFrameComponent;
class ESP8266Component;
class APMHeliComponent;
class APMRemoteSupportComponent;
class APMFollowComponent;
class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(APMAutoPilotPluginLog)

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
    APMCameraComponent *_cameraComponent = nullptr;
    APMLightsComponent *_lightsComponent = nullptr;
    APMSubFrameComponent *_subFrameComponent = nullptr;
    APMFlightModesComponent *_flightModesComponent = nullptr;
    APMPowerComponent *_powerComponent = nullptr;
    APMMotorComponent *_motorComponent = nullptr;
    APMRadioComponent *_radioComponent = nullptr;
    APMSafetyComponent *_safetyComponent = nullptr;
    APMSensorsComponent *_sensorsComponent = nullptr;
    APMTuningComponent *_tuningComponent = nullptr;
    ESP8266Component *_esp8266Component = nullptr;
    APMHeliComponent *_heliComponent = nullptr;
    APMRemoteSupportComponent *_apmRemoteSupportComponent = nullptr;
    APMFollowComponent *_followComponent = nullptr;

#ifndef QGC_NO_SERIAL_LINK
private slots:
    /// Executed when the Vehicle is parameter ready. It checks for the service bulletin against Cube Blacks.
    void _checkForBadCubeBlack(bool parametersReady);
#endif

private:
    QVariantList _components;
};
