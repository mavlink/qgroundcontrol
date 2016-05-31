/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "PX4RadioComponent.h"
#include "PX4AutoPilotPlugin.h"

PX4RadioComponent::PX4RadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Radio"))
{
}

QString PX4RadioComponent::name(void) const
{
    return _name;
}

QString PX4RadioComponent::description(void) const
{
    return tr("The Radio Component is used to setup which channels on your RC Transmitter you will use for each vehicle control such as Roll, Pitch, Yaw and Throttle. "
              "It also allows you to assign switches and dials to the various flight modes. "
              "Prior to flight you must also calibrate the extents for all of your channels.");
}

QString PX4RadioComponent::iconResource(void) const
{
    return "/qmlimages/RadioComponentIcon.png";
}

bool PX4RadioComponent::requiresSetup(void) const
{
    return _autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1 ? false : true;
}

bool PX4RadioComponent::setupComplete(void) const
{
    if (_autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
        // The best we can do to detect the need for a radio calibration is look for attitude
        // controls to be mapped.
        QStringList attitudeMappings;
        attitudeMappings << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
        foreach(const QString &mapParam, attitudeMappings) {
            if (_autopilot->getParameterFact(FactSystem::defaultComponentId, mapParam)->rawValue().toInt() == 0) {
                return false;
            }
        }
    }
    
    return true;
}

QStringList PX4RadioComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << "COM_RC_IN_MODE" << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
    
    return triggers;
}

QUrl PX4RadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/RadioComponent.qml");
}

QUrl PX4RadioComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/PX4RadioComponentSummary.qml");
}

QString PX4RadioComponent::prerequisiteSetup(void) const
{
    if (_autopilot->getParameterFact(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
        PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
        
        if (!plugin->airframeComponent()->setupComplete()) {
            return plugin->airframeComponent()->name();

        }
    }
    
    return QString();
}
