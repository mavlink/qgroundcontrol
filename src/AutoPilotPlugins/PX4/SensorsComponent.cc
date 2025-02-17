/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SensorsComponent.h"
#include "ParameterManager.h"
#include "Vehicle.h"

SensorsComponent::SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownSensorsVehicleComponent, parent)
    , _name(tr("Sensors"))
{
    _deviceIds = QStringList({QStringLiteral("CAL_GYRO0_ID"), QStringLiteral("CAL_ACC0_ID") });

    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        _airspeedCalTriggerParams << "SENS_DPRES_OFF";
        if (_vehicle->firmwareMajorVersion() >= 1 && _vehicle->firmwareMinorVersion() >= 14) {
            _airspeedCalTriggerParams << "SYS_HAS_NUM_ASPD";
        } else {
            _airspeedCalTriggerParams << "FW_ARSP_MODE" << "CBRK_AIRSPD_CHK";
        }
    }
}

QString SensorsComponent::name(void) const
{
    return _name;
}

QString SensorsComponent::description(void) const
{
    return tr("Sensors Setup is used to calibrate the sensors within your vehicle.");
}

QString SensorsComponent::iconResource(void) const
{
    return "/qmlimages/SensorsComponentIcon.png";
}

bool SensorsComponent::requiresSetup(void) const
{
    return true;
}

bool SensorsComponent::setupComplete(void) const
{
    foreach (const QString &triggerParam, _deviceIds) {
        if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, triggerParam)->rawValue().toFloat() == 0.0f) {
            return false;
        }
    }
    bool magEnabled = true;
    if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, _magEnabledParam)) {
        magEnabled = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, _magEnabledParam)->rawValue().toBool();
    }

    if (magEnabled && _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, _magCalParam)->rawValue().toFloat() == 0.0f) {
        return false;
    }

    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        if (_vehicle->firmwareMajorVersion() > 1 || (_vehicle->firmwareMajorVersion() == 1 && _vehicle->firmwareMinorVersion() > 14)) {
            if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "SYS_HAS_NUM_ASPD")->rawValue().toBool() &&
                    _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "SENS_DPRES_OFF")->rawValue().toFloat() == 0.0f) {
                return false;
            }
        } else {
            if (!_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "FW_ARSP_MODE")->rawValue().toBool() &&
                    _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "CBRK_AIRSPD_CHK")->rawValue().toInt() != 162128 &&
                    _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "SENS_DPRES_OFF")->rawValue().toFloat() == 0.0f) {
                return false;
            }
        }
    }

    return true;
}

QStringList SensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << _deviceIds << _magCalParam << _magEnabledParam;
    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        triggers << _airspeedCalTriggerParams;
    }
    
    return triggers;
}

QUrl SensorsComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/SensorsComponent.qml");
}

QUrl SensorsComponent::summaryQmlSource(void) const
{
    QString summaryQml;
    
    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        summaryQml = "qrc:/qml/SensorsComponentSummaryFixedWing.qml";
    } else {
        summaryQml = "qrc:/qml/SensorsComponentSummary.qml";
    }
    
    return QUrl::fromUserInput(summaryQml);
}

 bool SensorsComponent::_airspeedCalSupported(void) const
 {
    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        if (_vehicle->firmwareMajorVersion() > 1 || (_vehicle->firmwareMajorVersion() == 1 && _vehicle->firmwareMinorVersion() > 14)) {
            if (_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "SYS_HAS_NUM_ASPD")->rawValue().toBool()) {
                return true;
            }
        } else {
            if (!_vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "FW_ARSP_MODE")->rawValue().toBool() &&
                    _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "CBRK_AIRSPD_CHK")->rawValue().toInt() != 162128) {
                return true;
            }
        }
    }

    return false;
 }

 bool SensorsComponent::_airspeedCalRequired(void) const
 {
    return _airspeedCalSupported() && _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "SENS_DPRES_OFF")->rawValue().toFloat() == 0.0f;
 }
