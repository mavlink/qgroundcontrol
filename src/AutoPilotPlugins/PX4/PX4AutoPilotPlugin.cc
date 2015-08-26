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
#include "UASManager.h"
#include "PX4ParameterLoader.h"
#include "PX4AirframeLoader.h"
#include "FlightModesComponentController.h"
#include "AirframeComponentController.h"
#include "QGCMessageBox.h"

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
};

enum PX4_CUSTOM_SUB_MODE_AUTO {
    PX4_CUSTOM_SUB_MODE_AUTO_READY = 1,
    PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,
    PX4_CUSTOM_SUB_MODE_AUTO_LOITER,
    PX4_CUSTOM_SUB_MODE_AUTO_MISSION,
    PX4_CUSTOM_SUB_MODE_AUTO_RTL,
    PX4_CUSTOM_SUB_MODE_AUTO_LAND,
    PX4_CUSTOM_SUB_MODE_AUTO_RTGS
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

PX4AutoPilotPlugin::PX4AutoPilotPlugin(UASInterface* uas, QObject* parent) :
    AutoPilotPlugin(uas, parent),
    _parameterFacts(NULL),
    _airframeComponent(NULL),
    _radioComponent(NULL),
    _flightModesComponent(NULL),
    _sensorsComponent(NULL),
    _safetyComponent(NULL),
    _powerComponent(NULL),
    _incorrectParameterVersion(false)
{
    Q_ASSERT(uas);
    
    _parameterFacts = new PX4ParameterLoader(this, uas, this);
    Q_CHECK_PTR(_parameterFacts);
    
    connect(_parameterFacts, &PX4ParameterLoader::parametersReady, this, &PX4AutoPilotPlugin::_pluginReadyPreChecks);
    connect(_parameterFacts, &PX4ParameterLoader::parameterListProgress, this, &PX4AutoPilotPlugin::parameterListProgress);

    _airframeFacts = new PX4AirframeLoader(this, uas, this);
    Q_CHECK_PTR(_airframeFacts);
    
    PX4ParameterLoader::loadParameterFactMetaData();
    PX4AirframeLoader::loadAirframeFactMetaData();
}

PX4AutoPilotPlugin::~PX4AutoPilotPlugin()
{
    delete _parameterFacts;
    delete _airframeFacts;
}

void PX4AutoPilotPlugin::clearStaticData(void)
{
    PX4ParameterLoader::clearStaticData();
    PX4AirframeLoader::clearStaticData();
}

const QVariantList& PX4AutoPilotPlugin::vehicleComponents(void)
{
    if (_components.count() == 0 && !_incorrectParameterVersion) {
        Q_ASSERT(_uas);
        
        if (pluginReady()) {
            bool noRCTransmitter = false;
            if (parameterExists(FactSystem::defaultComponentId, "COM_RC_IN_MODE")) {
                Fact* rcFact = getParameterFact(FactSystem::defaultComponentId, "COM_RC_IN_MODE");
                noRCTransmitter = rcFact->value().toInt() == 1;
            }

            _airframeComponent = new AirframeComponent(_uas, this);
            Q_CHECK_PTR(_airframeComponent);
            _airframeComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_airframeComponent));
            
            if (!noRCTransmitter) {
                _radioComponent = new RadioComponent(_uas, this);
                Q_CHECK_PTR(_radioComponent);
                _radioComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_radioComponent));
                
                _flightModesComponent = new FlightModesComponent(_uas, this);
                Q_CHECK_PTR(_flightModesComponent);
                _flightModesComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_flightModesComponent));
            }
            
            _sensorsComponent = new SensorsComponent(_uas, this);
            Q_CHECK_PTR(_sensorsComponent);
            _sensorsComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));
            
            _powerComponent = new PowerComponent(_uas, this);
            Q_CHECK_PTR(_powerComponent);
            _powerComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_powerComponent));
            
            _safetyComponent = new SafetyComponent(_uas, this);
            Q_CHECK_PTR(_safetyComponent);
            _safetyComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue((VehicleComponent*)_safetyComponent));
        } else {
            qWarning() << "Call to vehicleCompenents prior to pluginReady";
        }
    }
    
    return _components;
}

/// This will perform various checks prior to signalling that the plug in ready
void PX4AutoPilotPlugin::_pluginReadyPreChecks(void)
{
    // Check for older parameter version set
    // FIXME: Firmware is moving to version stamp parameter set. Once that is complete the version stamp
    // should be used instead.
    if (parameterExists(FactSystem::defaultComponentId, "SENS_GYRO_XOFF")) {
        _incorrectParameterVersion = true;
        QGCMessageBox::warning("Setup", "This version of GroundControl can only perform vehicle setup on a newer version of firmware. "
										"Please perform a Firmware Upgrade if you wish to use Vehicle Setup.");
	}
	
    _pluginReady = true;
    emit pluginReadyChanged(_pluginReady);
}
