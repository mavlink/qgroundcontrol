/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SensorsComponent.h"
#include "PX4AutoPilotPlugin.h"
#include "QGCQmlWidgetHolder.h"
#include "SensorsComponentController.h"

const char* SensorsComponent::_airspeedBreaker =    "CBRK_AIRSPD_CHK";
const char* SensorsComponent::_airspeedCal =        "SENS_DPRES_OFF";

SensorsComponent::SensorsComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Sensors"))
{
    _deviceIds << QStringLiteral("CAL_MAG0_ID") << QStringLiteral("CAL_GYRO0_ID") << QStringLiteral("CAL_ACC0_ID");
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
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, triggerParam)->rawValue().toFloat() == 0.0f) {
            return false;
        }
    }

    if (_vehicle->fixedWing() || _vehicle->vtol()) {
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, _airspeedBreaker)->rawValue().toInt() != 162128) {
            if (_autopilot->getParameterFact(FactSystem::defaultComponentId, _airspeedCal)->rawValue().toFloat() == 0.0f) {
                return false;
            }
        }
    }

    return true;
}

QStringList SensorsComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << _deviceIds;
    if (_vehicle->fixedWing() || _vehicle->vtol()) {
        triggers << _airspeedCal << _airspeedBreaker;
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
    
    if (_vehicle->fixedWing() || _vehicle->vtol()) {
        summaryQml = "qrc:/qml/SensorsComponentSummaryFixedWing.qml";
    } else {
        summaryQml = "qrc:/qml/SensorsComponentSummary.qml";
    }
    
    return QUrl::fromUserInput(summaryQml);
}

QString SensorsComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    
    return QString();
}
