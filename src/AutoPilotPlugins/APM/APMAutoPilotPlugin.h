/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMAutoPilotPlugin_H
#define APMAutoPilotPlugin_H

#include "AutoPilotPlugin.h"
#include "Vehicle.h"

class APMAirframeComponent;
class APMAirframeLoader;
class APMFlightModesComponent;
class APMRadioComponent;
class APMTuningComponent;
class APMSafetyComponent;
class APMSensorsComponent;
class APMPowerComponent;
class MotorComponent;
class APMCameraComponent;
class APMLightsComponent;
class APMSubFrameComponent;
class ESP8266Component;

/// This is the APM specific implementation of the AutoPilot class.
class APMAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    ~APMAutoPilotPlugin();

    // Overrides from AutoPilotPlugin
    const QVariantList& vehicleComponents(void) final;

    APMAirframeComponent*       airframeComponent   (void) const { return _airframeComponent; }
    APMCameraComponent*         cameraComponent     (void) const { return _cameraComponent; }
    APMLightsComponent*         lightsComponent     (void) const { return _lightsComponent; }
    APMSubFrameComponent*       subFrameComponent   (void) const { return _subFrameComponent; }
    APMFlightModesComponent*    flightModesComponent(void) const { return _flightModesComponent; }
    APMPowerComponent*          powerComponent      (void) const { return _powerComponent; }
#if 0
    // Temporarily removed, waiting for new command implementation
    MotorComponent*             motorComponent      (void) const { return _motorComponent; }
#endif
    APMRadioComponent*          radioComponent      (void) const { return _radioComponent; }
    APMSafetyComponent*         safetyComponent     (void) const { return _safetyComponent; }
    APMSensorsComponent*        sensorsComponent    (void) const { return _sensorsComponent; }
    APMTuningComponent*         tuningComponent     (void) const { return _tuningComponent; }
    ESP8266Component*           esp8266Component    (void) const { return _esp8266Component; }

private:
    bool                    _incorrectParameterVersion; ///< true: parameter version incorrect, setup not allowed
    QVariantList            _components;

    APMAirframeComponent*       _airframeComponent;
    APMCameraComponent*         _cameraComponent;
    APMLightsComponent*         _lightsComponent;
    APMSubFrameComponent*       _subFrameComponent;
    APMFlightModesComponent*    _flightModesComponent;
    APMPowerComponent*          _powerComponent;
#if 0
    // Temporarily removed, waiting for new command implementation
    MotorComponent*             _motorComponent;
#endif
    APMRadioComponent*          _radioComponent;
    APMSafetyComponent*         _safetyComponent;
    APMSensorsComponent*        _sensorsComponent;
    APMTuningComponent*         _tuningComponent;
    APMAirframeLoader*          _airframeFacts;
    ESP8266Component*           _esp8266Component;
};

#endif
