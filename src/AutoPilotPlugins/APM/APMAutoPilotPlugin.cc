#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"
#include "APMAirspeedComponent.h"
#include "APMGimbalComponent.h"
#include "APMFlightModesComponent.h"
#include "APMHeliComponent.h"
#include "APMLightsComponent.h"
#include "APMMotorComponent.h"
#include "APMServoComponent.h"
#include "APMESCComponent.h"
#include "APMPowerComponent.h"
#include "APMRadioComponent.h"
#include "APMRemoteSupportComponent.h"
#include "APMFailsafesComponent.h"
#include "APMFlightSafetyComponent.h"
#include "APMSensorsComponent.h"
#include "APMSubFrameComponent.h"
#include "APMTuningComponent.h"
#include "APMAdvancedTuningCopterComponent.h"
#include "ESP8266Component.h"
#include "ScriptingComponent.h"
#include "JoystickComponent.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleSupports.h"
#include "VehicleComponent.h"

#include <algorithm>

#ifdef QT_DEBUG
#include "APMFollowComponent.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"
#endif
#ifndef QGC_NO_SERIAL_LINK
#include "QGCSerialPortInfo.h"
#include "SerialLink.h"
#endif

QGC_LOGGING_CATEGORY(APMAutoPilotPluginLog, "AutoPilotPlugins.APM.apmautopilotplugin")

APMAutoPilotPlugin::APMAutoPilotPlugin(Vehicle *vehicle, QObject *parent)
    : AutoPilotPlugin(vehicle, parent)
{
    // qCDebug(APMAutoPilotPluginLog) << Q_FUNC_INFO << this;

#ifndef QGC_NO_SERIAL_LINK
    (void) connect(vehicle->parameterManager(), &ParameterManager::parametersReadyChanged, this, &APMAutoPilotPlugin::_checkForBadCubeBlack);
#endif
}

APMAutoPilotPlugin::~APMAutoPilotPlugin()
{
    // qCDebug(APMAutoPilotPluginLog) << Q_FUNC_INFO << this;
}

const QVariantList &APMAutoPilotPlugin::vehicleComponents()
{
    if (_components.isEmpty() && !_incorrectParameterVersion) {
        if (_vehicle->parameterManager()->parametersReady()) {
            _airframeComponent = new APMAirframeComponent(_vehicle, this);
            _airframeComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_airframeComponent)));

            if (_vehicle->supports()->radio()) {
                _radioComponent = new APMRadioComponent(_vehicle, this);
                _radioComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_radioComponent)));
            }

            // No flight modes component for Sub versions 3.5 and up
            if (!_vehicle->sub() || (_vehicle->versionCompare(3, 5, 0) < 0)) {
                _flightModesComponent = new APMFlightModesComponent(_vehicle, this);
                _flightModesComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_flightModesComponent)));
            }

            _sensorsComponent = new APMSensorsComponent(_vehicle, this);
            _sensorsComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_sensorsComponent)));

            if (_vehicle->parameterManager()->parameterExists(-1, QStringLiteral("ARSPD_TYPE"))) {
                _airspeedComponent = new APMAirspeedComponent(_vehicle, this);
                _airspeedComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_airspeedComponent)));
            }

            _powerComponent = new APMPowerComponent(_vehicle, this);
            _powerComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_powerComponent)));

            _escComponent = new APMESCComponent(_vehicle, this);
            _escComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_escComponent)));

            if (!_vehicle->sub() || (_vehicle->sub() && (_vehicle->versionCompare(3, 5, 3) >= 0))) {
                _motorComponent = new APMMotorComponent(_vehicle, this);
                _motorComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_motorComponent)));
            }

            if (_vehicle->parameterManager()->parameterExists(-1, QStringLiteral("SERVO1_MIN"))) {
                _servoComponent = new APMServoComponent(_vehicle, this);
                _servoComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_servoComponent)));
            }

            _flightSafetyComponent = new APMFlightSafetyComponent(_vehicle, this);
            _flightSafetyComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_flightSafetyComponent)));

            _failsafesComponent = new APMFailsafesComponent(_vehicle, this);
            _failsafesComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_failsafesComponent)));

#ifdef QT_DEBUG
            if ((qobject_cast<ArduCopterFirmwarePlugin*>(_vehicle->firmwarePlugin()) || qobject_cast<ArduRoverFirmwarePlugin*>(_vehicle->firmwarePlugin())) &&
                    _vehicle->parameterManager()->parameterExists(-1, QStringLiteral("FOLL_ENABLE"))) {
                _followComponent = new APMFollowComponent(_vehicle, this);
                _followComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_followComponent)));
            }
#endif

            if (_vehicle->vehicleType() == MAV_TYPE_HELICOPTER && (_vehicle->versionCompare(4, 0, 0) >= 0)) {
                _heliComponent = new APMHeliComponent(_vehicle, this);
                _heliComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_heliComponent)));
            }

            _tuningComponent = new APMTuningComponent(_vehicle, this);
            _tuningComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_tuningComponent)));

            if (_vehicle->multiRotor()) {
                _advancedTuningCopterComponent = new APMAdvancedTuningCopterComponent(_vehicle, this);
                _advancedTuningCopterComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_advancedTuningCopterComponent)));
            }

            _gimbalComponent = new APMGimbalComponent(_vehicle, this);
            _gimbalComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_gimbalComponent)));

            if (_vehicle->sub()) {
                _lightsComponent = new APMLightsComponent(_vehicle, this);
                _lightsComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_lightsComponent)));

                if (_vehicle->versionCompare(3, 5, 0) >= 0) {
                    _subFrameComponent = new APMSubFrameComponent(_vehicle, this);
                    _subFrameComponent->setupTriggerSignals();
                    _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_subFrameComponent)));
                }
            }

            //-- Is there an ESP8266 Connected?
            if (_vehicle->parameterManager()->parameterExists(MAV_COMP_ID_UDP_BRIDGE, "SW_VER")) {
                _esp8266Component = new ESP8266Component(_vehicle, this);
                _esp8266Component->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_esp8266Component)));
            }

            _apmRemoteSupportComponent = new APMRemoteSupportComponent(_vehicle, this);
            _apmRemoteSupportComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_apmRemoteSupportComponent)));

            _joystickComponent = new JoystickComponent(_vehicle, this, this);
            _joystickComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_joystickComponent)));

            _scriptingComponent = new ScriptingComponent(_vehicle, this, this);
            _scriptingComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_scriptingComponent)));
        } else {
            qCWarning(APMAutoPilotPluginLog) << "Call to vehicleComponents prior to parametersReady";
        }

        std::sort(_components.begin(), _components.end(), [](const QVariant &a, const QVariant &b) {
            return a.value<VehicleComponent*>()->name().toLower() < b.value<VehicleComponent*>()->name().toLower();
        });
    }

    return _components;
}

QString APMAutoPilotPlugin::prerequisiteSetup(VehicleComponent *component) const
{
    bool requiresAirframeCheck = false;

    if (qobject_cast<const APMFlightModesComponent*>(component)) {
        if (_airframeComponent && !_airframeComponent->setupComplete()) {
            return _airframeComponent->name();
        }
        if (_radioComponent && !_radioComponent->setupComplete()) {
            return _radioComponent->name();
        }
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMRadioComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMPowerComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMESCComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMFlightSafetyComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMTuningComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMSensorsComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMAirspeedComponent*>(component)) {
        requiresAirframeCheck = true;
    }

    if (requiresAirframeCheck) {
        if (_airframeComponent && !_airframeComponent->setupComplete()) {
            return _airframeComponent->name();
        }
    }

    return QString();
}

#ifndef QGC_NO_SERIAL_LINK
void APMAutoPilotPlugin::_checkForBadCubeBlack(bool parametersReady)
{
    if (!parametersReady) {
        return;
    }

    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    if (sharedLink->linkConfiguration()->type() != LinkConfiguration::TypeSerial) {
        return;
    }

    const SerialLink *serialLink = qobject_cast<const SerialLink*>(sharedLink.get());
    if (!serialLink) {
        return;
    }

    if (!QGCSerialPortInfo(*serialLink->port()).isBlackCube()) {
        return;
    }

    ParameterManager *const paramMgr = _vehicle->parameterManager();

    static const QString paramAcc3 = QStringLiteral("INS_ACC3_ID");
    static const QString paramGyr3 = QStringLiteral("INS_GYR3_ID");
    static const QString paramEnableMask = QStringLiteral("INS_ENABLE_MASK");

    if (paramMgr->parameterExists(-1, paramAcc3) && (paramMgr->getParameter(-1, paramAcc3)->rawValue().toInt() == 0) &&
        paramMgr->parameterExists(-1, paramGyr3) && (paramMgr->getParameter(-1, paramGyr3)->rawValue().toInt() == 0) &&
        paramMgr->parameterExists(-1, paramEnableMask) && (paramMgr->getParameter(-1, paramEnableMask)->rawValue().toInt() >= 7)) {
        qgcApp()->showAppMessage(tr(
            "WARNING: The flight board you are using has a critical service bulletin against it which advises against flying. "
            "For details see: https://discuss.cubepilot.org/t/sb-0000002-critical-service-bulletin-for-cubes-purchased-between-january-2019-to-present-do-not-fly/406"
        ));
    }
}
#endif
