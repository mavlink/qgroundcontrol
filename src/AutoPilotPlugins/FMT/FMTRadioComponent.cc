/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FMTRadioComponent.h"
#include "FMTAutoPilotPlugin.h"

FMTRadioComponent::FMTRadioComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Radio"))
{
}

QString FMTRadioComponent::name(void) const
{
    return _name;
}

QString FMTRadioComponent::description(void) const
{
    return tr("Radio Setup is used to calibrate your transmitter. "
              "It also assign channels for Roll, Pitch, Yaw and Throttle vehicle control as well as determining whether they are reversed.");
}

QString FMTRadioComponent::iconResource(void) const
{
    return "/qmlimages/RadioComponentIcon.png";
}

bool FMTRadioComponent::requiresSetup(void) const
{
    return _vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() == 1 ? false : true;
}

bool FMTRadioComponent::setupComplete(void) const
{
    if (_vehicle->parameterManager()->getParameter(-1, "COM_RC_IN_MODE")->rawValue().toInt() != 1) {
        // The best we can do to detect the need for a radio calibration is look for attitude
        // controls to be mapped.
        QStringList attitudeMappings;
        attitudeMappings << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
        foreach(const QString &mapParam, attitudeMappings) {
            if (_vehicle->parameterManager()->getParameter(FactSystem::defaultComponentId, mapParam)->rawValue().toInt() == 0) {
                return false;
            }
        }
    }
    
    return true;
}

QStringList FMTRadioComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    triggers << "COM_RC_IN_MODE" << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
    
    return triggers;
}

QUrl FMTRadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/RadioComponent.qml");
}

QUrl FMTRadioComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/FMTRadioComponentSummary.qml");
}
