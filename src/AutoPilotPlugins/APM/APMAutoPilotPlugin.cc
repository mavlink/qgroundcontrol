/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"
#include "APMCameraComponent.h"
#include "APMFlightModesComponent.h"
#include "APMHeliComponent.h"
#include "APMLightsComponent.h"
#include "APMMotorComponent.h"
#include "APMPowerComponent.h"
#include "APMRadioComponent.h"
#include "APMRemoteSupportComponent.h"
#include "APMSafetyComponent.h"
#include "APMSensorsComponent.h"
#include "APMSubFrameComponent.h"
#include "APMTuningComponent.h"
#include "ESP8266Component.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleComponent.h"
#ifdef QT_DEBUG
#include "APMFollowComponent.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"
#endif
#ifndef QGC_NO_SERIAL_LINK
#include "QGCSerialPortInfo.h"
#include "SerialLink.h"
#endif

QGC_LOGGING_CATEGORY(APMAutoPilotPluginLog, "qgc.autopilotplugins.apm.apmautopilotplugin")

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

            if (_vehicle->supportsRadio()) {
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

            _powerComponent = new APMPowerComponent(_vehicle, this);
            _powerComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_powerComponent)));

            if (!_vehicle->sub() || (_vehicle->sub() && (_vehicle->versionCompare(3, 5, 3) >= 0))) {
                _motorComponent = new APMMotorComponent(_vehicle, this);
                _motorComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_motorComponent)));
            }

            _safetyComponent = new APMSafetyComponent(_vehicle, this);
            _safetyComponent->setupTriggerSignals();
            _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_safetyComponent)));

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

            if (_vehicle->parameterManager()->parameterExists(-1, "MNT1_TYPE")) {
                _cameraComponent = new APMCameraComponent(_vehicle, this);
                _cameraComponent->setupTriggerSignals();
                _components.append(QVariant::fromValue(qobject_cast<VehicleComponent*>(_cameraComponent)));
            }

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
        } else {
            qCWarning(APMAutoPilotPluginLog) << "Call to vehicleComponents prior to parametersReady";
        }
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
    } else if (qobject_cast<const APMCameraComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMPowerComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMSafetyComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMTuningComponent*>(component)) {
        requiresAirframeCheck = true;
    } else if (qobject_cast<const APMSensorsComponent*>(component)) {
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
