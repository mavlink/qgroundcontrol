/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom Autopilot Plugin
 *   @author Gus Grubba <gus@auterion.com>
 */

#include "CustomAutoPilotPlugin.h"

#include "QGCApplication.h"
#include "QGCCorePlugin.h"

//-----------------------------------------------------------------------------
CustomAutoPilotPlugin::CustomAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : PX4AutoPilotPlugin(vehicle, parent)
{
    connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &CustomAutoPilotPlugin::_advancedChanged);
}

//-----------------------------------------------------------------------------
void
CustomAutoPilotPlugin::_advancedChanged(bool)
{
    _components.clear();
    emit vehicleComponentsChanged();
}

//-----------------------------------------------------------------------------
const QVariantList&
CustomAutoPilotPlugin::vehicleComponents()
{
    if (_components.count() == 0 && !_incorrectParameterVersion) {
        if (_vehicle) {
            bool showAdvanced = qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
            qDebug() << "Loading components:" << showAdvanced;
            if (_vehicle->parameterManager()->parametersReady()) {
                if(showAdvanced) {
                _airframeComponent = new AirframeComponent(_vehicle, this);
                _airframeComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_airframeComponent)));
                }
                if (!_vehicle->hilMode()) {
                    _sensorsComponent = new SensorsComponent(_vehicle, this);
                    _sensorsComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_sensorsComponent)));
                }
                if(showAdvanced) {
                    _radioComponent = new PX4RadioComponent(_vehicle, this);
                    _radioComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_radioComponent)));

                    _flightModesComponent = new FlightModesComponent(_vehicle, this);
                    _flightModesComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_flightModesComponent)));

                    _powerComponent = new PowerComponent(_vehicle, this);
                    _powerComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_powerComponent)));

                    _motorComponent = new MotorComponent(_vehicle, this);
                    _motorComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_motorComponent)));
                }

                _safetyComponent = new SafetyComponent(_vehicle, this);
                _safetyComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_safetyComponent)));

                if(showAdvanced) {
                    _tuningComponent = new PX4TuningComponent(_vehicle, this);
                    _tuningComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_tuningComponent)));

                    //-- Is there support for cameras?
                    if(_vehicle->parameterManager()->parameterExists(_vehicle->id(), "TRIG_MODE")) {
                        _cameraComponent = new CameraComponent(_vehicle, this);
                        _cameraComponent->setupTriggerSignals();
                        _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_cameraComponent)));
                    }
                }

                //-- Is there an ESP8266 Connected?
                if(_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_UDP_BRIDGE, "SW_VER")) {
                    _esp8266Component = new ESP8266Component(_vehicle, this);
                    _esp8266Component->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_esp8266Component)));
                }
            } else {
                qWarning() << "Call to vehicleCompenents prior to parametersReady";
            }

            if(_vehicle->parameterManager()->parameterExists(_vehicle->id(), "SLNK_RADIO_CHAN")) {
                _syslinkComponent = new SyslinkComponent(_vehicle, this);
                _syslinkComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_syslinkComponent)));
            }
        } else {
            qWarning() << "Internal error";
        }
    }
    return _components;
}
