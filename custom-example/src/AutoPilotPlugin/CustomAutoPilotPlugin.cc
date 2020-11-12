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

CustomAutoPilotPlugin::CustomAutoPilotPlugin(Vehicle* vehicle, QObject* parent)
    : PX4AutoPilotPlugin(vehicle, parent)
{
    // Whenever we go on/out of advanced mode the available list of settings pages will change
    connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &CustomAutoPilotPlugin::_advancedChanged);
}

// This signals that when Advanced Mode changes the list of Vehicle Settings page also changed
void CustomAutoPilotPlugin::_advancedChanged(bool)
{
    _components.clear();
    emit vehicleComponentsChanged();
}

// This allows us to hide most Vehicle Setup pages unless we are in Advanced Mmode
const QVariantList& CustomAutoPilotPlugin::vehicleComponents()
{
    if (_components.count() == 0 && !_incorrectParameterVersion) {
        if (_vehicle) {
            bool showAdvanced = qgcApp()->toolbox()->corePlugin()->showAdvancedUI();
            if (_vehicle->parameterManager()->parametersReady()) {
                if (showAdvanced) {
                    _airframeComponent = new AirframeComponent(_vehicle, this);
                    _airframeComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_airframeComponent)));

                    _sensorsComponent = new SensorsComponent(_vehicle, this);
                    _sensorsComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(reinterpret_cast<VehicleComponent*>(_sensorsComponent)));

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

                if (showAdvanced) {
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
            } else {
                qWarning() << "Call to vehicleCompenents prior to parametersReady";
            }
        } else {
            qWarning() << "Internal error";
        }
    }
    return _components;
}
