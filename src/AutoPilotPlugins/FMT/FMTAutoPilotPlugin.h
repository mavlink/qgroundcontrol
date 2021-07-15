/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef FMTAUTOPILOT_H
#define FMTAUTOPILOT_H

#include "AutoPilotPlugin.h"
#include "FMTAirframeLoader.h"
#include "AirframeComponent.h"
#include "FMTRadioComponent.h"
#include "ESP8266Component.h"
#include "FlightModesComponent.h"
#include "SensorsComponent.h"
#include "SafetyComponent.h"
#include "CameraComponent.h"
#include "PowerComponent.h"
#include "MotorComponent.h"
#include "FMTTuningComponent.h"
#include "SyslinkComponent.h"
#include "Vehicle.h"

#include <QImage>

/// @file
///     @brief This is the FMT specific implementation of the AutoPilot class.
///     @author Don Gagne <don@thegagnes.com>

class FMTAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    FMTAutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    ~FMTAutoPilotPlugin();

    // Overrides from AutoPilotPlugin
    const QVariantList& vehicleComponents(void) override;
    void parametersReadyPreChecks(void) override;
    QString prerequisiteSetup(VehicleComponent* component) const override;

protected:
    bool                    _incorrectParameterVersion; ///< true: parameter version incorrect, setup not allowed
    FMTAirframeLoader*      _airframeFacts;
    AirframeComponent*      _airframeComponent;
    FMTRadioComponent*      _radioComponent;
    ESP8266Component*       _esp8266Component;
    FlightModesComponent*   _flightModesComponent;
    SensorsComponent*       _sensorsComponent;
    SafetyComponent*        _safetyComponent;
    CameraComponent*        _cameraComponent;
    PowerComponent*         _powerComponent;
    MotorComponent*         _motorComponent;
    FMTTuningComponent*     _tuningComponent;
    SyslinkComponent*       _syslinkComponent;

private:
    QVariantList            _components;
};

#endif
