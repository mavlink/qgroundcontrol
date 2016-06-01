/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

            if (!_vehicle->hilMode()) {
                _sensorsComponent = new SensorsComponent(_vehicle, this);
                _sensorsComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_sensorsComponent));
            }

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

            //-- Is there support for cameras?
            if(factExists(FactSystem::ParameterProvider, _vehicle->id(), "TRIG_MODE")) {
                _cameraComponent = new CameraComponent(_vehicle, this);
                _cameraComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue((VehicleComponent*)_cameraComponent));
            }

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
    if (parameterExists(FactSystem::defaultComponentId, "SENS_GYRO_XOFF") ||
            parameterExists(FactSystem::defaultComponentId, "COM_DL_LOSS_EN")) {
        _incorrectParameterVersion = true;
        qgcApp()->showMessage("This version of GroundControl can only perform vehicle setup on a newer version of firmware. "
                              "Please perform a Firmware Upgrade if you wish to use Vehicle Setup.");
    }

    _parametersReady = true;
    _missingParameters = missingParameters;
    emit missingParametersChanged(_missingParameters);
    emit parametersReadyChanged(_parametersReady);
}
