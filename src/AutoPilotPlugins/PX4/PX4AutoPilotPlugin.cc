/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "PX4AutoPilotPlugin.h"
#include "PX4AirframeLoader.h"
#include "QGCApplication.h"
#include "FlightModesComponent.h"
#include "PX4RadioComponent.h"
#include "PX4TuningComponent.h"
#include "PowerComponent.h"
#include "SafetyComponent.h"
#include "SensorsComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "Actuators.h"
#include "ActuatorComponent.h"

/// @file
///     @brief This is the AutoPilotPlugin implementatin for the MAV_AUTOPILOT_PX4 type.
///     @author Don Gagne <don@thegagnes.com>

PX4AutoPilotPlugin::PX4AutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : AutoPilotPlugin(vehicle, parent)
    , _incorrectParameterVersion(false)
    , _airframeComponent(nullptr)
    , _radioComponent(nullptr)
    , _esp8266Component(nullptr)
    , _flightModesComponent(nullptr)
    , _sensorsComponent(nullptr)
    , _safetyComponent(nullptr)
    , _powerComponent(nullptr)
    , _motorComponent(nullptr)
    , _actuatorComponent(nullptr)
    , _tuningComponent(nullptr)
    , _flightBehavior(nullptr)
    , _syslinkComponent(nullptr)
{
    if (!vehicle) {
        qWarning() << "Internal error";
        return;
    }

    _airframeFacts = new PX4AirframeLoader(this, this);
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
        if (_vehicle) {
            if (_vehicle->parameterManager()->parametersReady()) {
                _airframeComponent = new AirframeComponent(_vehicle, this, this);
                _airframeComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_airframeComponent)));

                if (!_vehicle->hilMode()) {
                    _sensorsComponent = new SensorsComponent(_vehicle, this, this);
                    _sensorsComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_sensorsComponent)));
                }

                _radioComponent = new PX4RadioComponent(_vehicle, this, this);
                _radioComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_radioComponent)));

                _flightModesComponent = new FlightModesComponent(_vehicle, this, this);
                _flightModesComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_flightModesComponent)));

                _powerComponent = new PowerComponent(_vehicle, this, this);
                _powerComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_powerComponent)));

                if (_vehicle->actuators()) {
                    _vehicle->actuators()->init(); // At this point params are loaded, so we can init the actuators
                }
                if (_vehicle->actuators() && _vehicle->actuators()->showUi()) {
                    _actuatorComponent = new ActuatorComponent(_vehicle, this, this);
                    _actuatorComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_actuatorComponent)));
                } else {
                    // show previous motor UI instead
                    _motorComponent = new MotorComponent(_vehicle, this, this);
                    _motorComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_motorComponent)));
                }

                _safetyComponent = new SafetyComponent(_vehicle, this, this);
                _safetyComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_safetyComponent)));

                _tuningComponent = new PX4TuningComponent(_vehicle, this, this);
                _tuningComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_tuningComponent)));

                if(_vehicle->parameterManager()->parameterExists(_vehicle->compId(), "SYS_VEHICLE_RESP")) {
                    _flightBehavior = new PX4FlightBehavior(_vehicle, this, this);
                    _flightBehavior->setupTriggerSignals();
                    _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_flightBehavior)));
                }

                //-- Is there an ESP8266 Connected?
                if(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_UDP_BRIDGE, "SW_VER")) {
                    _esp8266Component = new ESP8266Component(_vehicle, this, this);
                    _esp8266Component->setupTriggerSignals();
                    _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_esp8266Component)));
                }
            } else {
                qWarning() << "Call to vehicleCompenents prior to parametersReady";
            }

            if(_vehicle->parameterManager()->parameterExists(_vehicle->compId(), "SLNK_RADIO_CHAN")) {
                _syslinkComponent = new SyslinkComponent(_vehicle, this, this);
                _syslinkComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(static_cast<VehicleComponent*>(_syslinkComponent)));
            }
        } else {
            qWarning() << "Internal error";
        }
    }

    return _components;
}

void PX4AutoPilotPlugin::parametersReadyPreChecks(void)
{
    // Base class must be called
    AutoPilotPlugin::parametersReadyPreChecks();

    QString hitlParam("SYS_HITL");
    if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, hitlParam) &&
            _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, hitlParam)->rawValue().toBool()) {
        qgcApp()->showAppMessage(tr("Warning: Hardware In The Loop (HITL) simulation is enabled for this vehicle."));
    }
}

QString PX4AutoPilotPlugin::prerequisiteSetup(VehicleComponent* component) const
{
    bool requiresAirframeCheck = false;

    if (qobject_cast<const FlightModesComponent*>(component)) {
        if (_vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1) {
            // No RC input
            return QString();
        } else {
            if (_airframeComponent && !_airframeComponent->setupComplete()) {
                return _airframeComponent->name();
            } else if (_radioComponent && !_radioComponent->setupComplete()) {
                return _radioComponent->name();
            }
        }
    } else if (qobject_cast<const PX4RadioComponent*>(component)) {
        if (_vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
            requiresAirframeCheck = true;
        }
    } else if (qobject_cast<const PX4TuningComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const PowerComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const SafetyComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const SensorsComponent*>(component)) {
        requiresAirframeCheck = true;
    }

    if (requiresAirframeCheck) {
        if (_airframeComponent && !_airframeComponent->setupComplete()) {
            return _airframeComponent->name();
        }
    }

    return QString();
}
