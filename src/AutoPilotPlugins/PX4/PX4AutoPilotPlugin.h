/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
#include "PowerComponent.h"
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
    PowerComponent*         powerComponent(void)        { return _powerComponent; }
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
    PowerComponent*         _powerComponent;
    PX4TuningComponent*     _tuningComponent;
    bool                    _incorrectParameterVersion; ///< true: parameter version incorrect, setup not allowed
};

#endif
