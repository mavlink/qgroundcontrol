/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef PX4AUTOPILOT_H
#define PX4AUTOPILOT_H

#include "AutoPilotPlugin.h"
#include "PX4AirframeLoader.h"
#include "AirframeComponent.h"
#include "PX4RadioComponent.h"
#include "ESP8266Component.h"
#include "FlightModesComponent.h"
#include "SensorsComponent.h"
#include "SafetyComponent.h"
#include "CameraComponent.h"
#include "PowerComponent.h"
#include "MotorComponent.h"
#include "PX4TuningComponent.h"
#include "Vehicle.h"

#include <QImage>

/// @file
///     @brief This is the PX4 specific implementation of the AutoPilot class.
///     @author Don Gagne <don@thegagnes.com>

class PX4AutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    PX4AutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    ~PX4AutoPilotPlugin();

    // Overrides from AutoPilotPlugin
    virtual const QVariantList& vehicleComponents(void);

    // These methods should only be used by objects within the plugin
    AirframeComponent*      airframeComponent(void)     { return _airframeComponent; }
    PX4RadioComponent*      radioComponent(void)        { return _radioComponent; }
    ESP8266Component*       esp8266Component(void)      { return _esp8266Component; }
    FlightModesComponent*   flightModesComponent(void)  { return _flightModesComponent; }
    SensorsComponent*       sensorsComponent(void)      { return _sensorsComponent; }
    SafetyComponent*        safetyComponent(void)       { return _safetyComponent; }
    CameraComponent*        cameraComponent(void)       { return _cameraComponent; }
    PowerComponent*         powerComponent(void)        { return _powerComponent; }
    MotorComponent*         motorComponent(void)        { return _motorComponent; }
    PX4TuningComponent*     tuningComponent(void)       { return _tuningComponent; }

public slots:
    // FIXME: This is public until we restructure AutoPilotPlugin/FirmwarePlugin/Vehicle
    void _parametersReadyPreChecks(bool missingParameters);

private:
    PX4AirframeLoader*      _airframeFacts;
    QVariantList            _components;
    AirframeComponent*      _airframeComponent;
    PX4RadioComponent*      _radioComponent;
    ESP8266Component*       _esp8266Component;
    FlightModesComponent*   _flightModesComponent;
    SensorsComponent*       _sensorsComponent;
    SafetyComponent*        _safetyComponent;
    CameraComponent*        _cameraComponent;
    PowerComponent*         _powerComponent;
    MotorComponent*         _motorComponent;
    PX4TuningComponent*     _tuningComponent;
    bool                    _incorrectParameterVersion; ///< true: parameter version incorrect, setup not allowed
};

#endif
