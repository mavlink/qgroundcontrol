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

#include "PX4AutoPilotPlugin.h"
#include "AutoPilotPluginManager.h"
#include "PX4AirframeLoader.h"
#include "PX4AdvancedFlightModesController.h"
#include "AirframeComponentController.h"
#include "UAS.h"
#include "FirmwarePlugin/PX4/PX4ParameterMetaData.h"  // FIXME: Hack
#include "FirmwarePlugin/PX4/PX4FirmwarePlugin.h"  // FIXME: Hack
#include "QGCApplication.h"

/// @file
///     @brief This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_PX4 type.
///     @author Don Gagne <don@thegagnes.com>

enum PX4_CUSTOM_MAIN_MODE {
    PX4_CUSTOM_MAIN_MODE_MANUAL = 1,
    PX4_CUSTOM_MAIN_MODE_ALTCTL,
    PX4_CUSTOM_MAIN_MODE_POSCTL,
    PX4_CUSTOM_MAIN_MODE_AUTO,
    PX4_CUSTOM_MAIN_MODE_ACRO,
    PX4_CUSTOM_MAIN_MODE_OFFBOARD,
    PX4_CUSTOM_MAIN_MODE_STABILIZED,
    PX4_CUSTOM_MAIN_MODE_RATTITUDE

};

enum PX4_CUSTOM_SUB_MODE_AUTO {
    PX4_CUSTOM_SUB_MODE_AUTO_READY = 1,
    PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,
    PX4_CUSTOM_SUB_MODE_AUTO_LOITER,
    PX4_CUSTOM_SUB_MODE_AUTO_MISSION,
    PX4_CUSTOM_SUB_MODE_AUTO_RTL,
    PX4_CUSTOM_SUB_MODE_AUTO_LAND,
    PX4_CUSTOM_SUB_MODE_AUTO_RTGS,
    PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_ME
};

union px4_custom_mode {
    struct {
        uint16_t reserved;
        uint8_t main_mode;
        uint8_t sub_mode;
    };
    uint32_t data;
    float data_float;
};

PX4AutoPilotPlugin::PX4AutoPilotPlugin(Vehicle* vehicle, QObject* parent) :
    AutoPilotPlugin(vehicle, parent),
    _airframeComponent(NULL),
    _radioComponent(NULL),
    _esp8266Component(NULL),
    _flightModesComponent(NULL),
    _sensorsComponent(NULL),
    _safetyComponent(NULL),
    _powerComponent(NULL),
    _incorrectParameterVersion(false)
{
    Q_ASSERT(vehicle);

    _airframeFacts = new PX4AirframeLoader(this, _vehicle->uas(), this);
    Q_CHECK_PTR(_airframeFacts);

    PX4AirframeLoader::loadAirframeMetaData();
}

PX4AutoPilotPlugin::~PX4AutoPilotPlugin()
{
    delete _airframeFacts;
}

const QVariantList& PX4AutoPilotPlugin::vehicleComponents(void)
{
    if (_components.count() == 0 && !_incorrectParameterVersion) {
        Q_ASSERT(_vehicle);

        if (parametersReady()) {
            _airframeComponent = new AirframeComponent(_vehicle, this);
            _airframeComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_airframeComponent));

            _radioComponent = new PX4RadioComponent(_vehicle, this);
            _radioComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_radioComponent));

            _sensorsComponent = new SensorsComponent(_vehicle, this);
            _sensorsComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));

            _flightModesComponent = new FlightModesComponent(_vehicle, this);
            _flightModesComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_flightModesComponent));

            _powerComponent = new PowerComponent(_vehicle, this);
            _powerComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_powerComponent));

            _safetyComponent = new SafetyComponent(_vehicle, this);
            _safetyComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_safetyComponent));

            _tuningComponent = new PX4TuningComponent(_vehicle, this);
            _tuningComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_tuningComponent));

            //-- Is there an ESP8266 Connected?
            if(factExists(FactSystem::ParameterProvider, MAV_COMP_ID_UDP_BRIDGE, "SW_VER")) {
                _esp8266Component = new ESP8266Component(_vehicle, this);
                _esp8266Component->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_esp8266Component));
            }
        } else {
            qWarning() << "Call to vehicleCompenents prior to parametersReady";
        }
    }

    return _components;
}

/// This will perform various checks prior to signalling that the plug in ready
void PX4AutoPilotPlugin::_parametersReadyPreChecks(bool missingParameters)
{
    // Check for older parameter version set
    // FIXME: Firmware is moving to version stamp parameter set. Once that is complete the version stamp
    // should be used instead.
    if (parameterExists(FactSystem::defaultComponentId, "SENS_GYRO_XOFF")) {
        _incorrectParameterVersion = true;
        qgcApp()->showMessage("This version of GroundControl can only perform vehicle setup on a newer version of firmware. "
                              "Please perform a Firmware Upgrade if you wish to use Vehicle Setup.");
    }

    _parametersReady = true;
    _missingParameters = missingParameters;
    emit missingParametersChanged(_missingParameters);
    emit parametersReadyChanged(_parametersReady);
}
