/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef APMAutoPilotPlugin_H
#define APMAutoPilotPlugin_H

#include "AutoPilotPlugin.h"
#include "Vehicle.h"
#include "APMAirframeComponent.h"
#include "APMRadioComponent.h"
#include "APMFlightModesComponent.h"

/// This is the APM specific implementation of the AutoPilot class.
class APMAutoPilotPlugin : public AutoPilotPlugin
{
    Q_OBJECT

public:
    APMAutoPilotPlugin(Vehicle* vehicle, QObject* parent);
    ~APMAutoPilotPlugin();

    // Overrides from AutoPilotPlugin
    virtual const QVariantList& vehicleComponents(void);

    APMAirframeComponent*       airframeComponent   (void) { return _airframeComponent; }
    APMFlightModesComponent*    flightModesComponent(void) { return _flightModesComponent; }
    APMRadioComponent*          radioComponent      (void) { return _radioComponent; }

public slots:
    // FIXME: This is public until we restructure AutoPilotPlugin/FirmwarePlugin/Vehicle
    void _parametersReadyPreChecks(bool missingParameters);

private:
    bool                    _incorrectParameterVersion; ///< true: parameter version incorrect, setup not allowed
    QVariantList            _components;

    APMAirframeComponent*       _airframeComponent;
    APMFlightModesComponent*    _flightModesComponent;
    APMRadioComponent*          _radioComponent;
};

#endif
