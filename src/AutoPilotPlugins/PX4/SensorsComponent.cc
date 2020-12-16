/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SensorsComponent.h"
#include "PX4AutoPilotPlugin.h"
#include "SensorsComponentController.h"

const char* SensorsComponent::_airspeedBreakerParam =   "CBRK_AIRSPD_CHK";
const char* SensorsComponent::_airspeedDisabledParam =  "FW_ARSP_MODE";
const char* SensorsComponent::_airspeedCalParam =       "SENS_DPRES_OFF";

const char* SensorsComponent::_magEnabledParam =  "SYS_HAS_MAG";
const char* SensorsComponent::_magCalParam =  "CAL_MAG0_ID";

SensorsComponent::SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Sensors"))
{
    _deviceIds = QStringList({QStringLiteral("CAL_GYRO0_ID"), QStringLiteral("CAL_ACC0_ID") });
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
        if (_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, triggerParam)->rawValue().toFloat() == 0.0f) {
            return false;
        }
    }
    bool magEnabled = true;
    if (_vehicle->parameterManager()->parameterExists(FactSystem::defaultComponentId, _magEnabledParam)) {
        magEnabled = _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _magEnabledParam)->rawValue().toBool();
    }

    if (magEnabled && _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _magCalParam)->rawValue().toFloat() == 0.0f) {
        return false;
    }

    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        if (!_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _airspeedDisabledParam)->rawValue().toBool() &&
                _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _airspeedBreakerParam)->rawValue().toInt() != 162128 &&
                _vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, _airspeedCalParam)->rawValue().toFloat() == 0.0f) {
            return false;
        }
    }

    return true;
}

QStringList SensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << _deviceIds << _magCalParam << _magEnabledParam;
    if (_vehicle->fixedWing() || _vehicle->vtol() || _vehicle->airship()) {
        triggers << _airspeedCalParam << _airspeedBreakerParam;
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
