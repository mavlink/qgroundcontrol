/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "OutputComponent.h"
#include "PowerComponent.h"
#include "MotorComponent.h"
#include "PX4TuningComponent.h"
#include "SyslinkComponent.h"
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
    const QVariantList& vehicleComponents(void) override;
    void parametersReadyPreChecks(void) override;
    QString prerequisiteSetup(VehicleComponent* component) const override;

protected:
    bool                    _incorrectParameterVersion  = false;    ///< true: parameter version incorrect, setup not allowed
    PX4AirframeLoader*      _airframeFacts              = nullptr;
    AirframeComponent*      _airframeComponent          = nullptr;
    PX4RadioComponent*      _radioComponent             = nullptr;
    ESP8266Component*       _esp8266Component           = nullptr;
    FlightModesComponent*   _flightModesComponent       = nullptr;
    SensorsComponent*       _sensorsComponent           = nullptr;
    SafetyComponent*        _safetyComponent            = nullptr;
    CameraComponent*        _cameraComponent            = nullptr;
    OutputComponent*        _outputComponent            = nullptr;
    PowerComponent*         _powerComponent             = nullptr;
    MotorComponent*         _motorComponent             = nullptr;
    PX4TuningComponent*     _tuningComponent            = nullptr;
    SyslinkComponent*       _syslinkComponent           = nullptr;

private:
    QVariantList            _components;
};

#endif
