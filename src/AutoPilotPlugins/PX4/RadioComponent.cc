/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "RadioComponent.h"
#include "QGCQmlWidgetHolder.h"
#include "PX4AutoPilotPlugin.h"

RadioComponent::RadioComponent(UASInterface* uas, AutoPilotPlugin* autopilot, QObject* parent) :
    PX4Component(uas, autopilot, parent),
    _name(tr("Radio"))
{
}

QString RadioComponent::name(void) const
{
    return _name;
}

QString RadioComponent::description(void) const
{
    return tr("The Radio Component is used to setup which channels on your RC Transmitter you will use for each vehicle control such as Roll, Pitch, Yaw and Throttle. "
              "It also allows you to assign switches and dials to the various flight modes. "
              "Prior to flight you must also calibrate the extents for all of your channels.");
}

QString RadioComponent::iconResource(void) const
{
    return "/qmlimages/RadioComponentIcon.png";
}

bool RadioComponent::requiresSetup(void) const
{
    return true;
}

bool RadioComponent::setupComplete(void) const
{
    // Check for mapped attitude controls
    QStringList attitudeMappings;
    attitudeMappings << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
    foreach(QString mapParam, attitudeMappings) {
        if (_autopilot->getParameterFact(FactSystem::defaultComponentId, mapParam)->value().toInt() == 0) {
            return false;
        }
    }
    
    // Check for min/max/trim defaults for channel 1-4
    
    static const int rcMinDefault = 1000;
    static const int rcMaxDefault = 2000;
    static const int rcTrimDefault = 1500;

    for (int i=1; i<5; i++) {
        QVariant value;
        int rcMin, rcMax, rcTrim;

        QString param;
        
        param = QString("RC%1_MIN").arg(i);
        rcMin = _autopilot->getParameterFact(FactSystem::defaultComponentId, param)->value().toInt();
        
        param = QString("RC%1_MAX").arg(i);
        rcMax = _autopilot->getParameterFact(FactSystem::defaultComponentId, param)->value().toInt();
        
        param = QString("RC%1_TRIM").arg(i);
        rcTrim = _autopilot->getParameterFact(FactSystem::defaultComponentId, param)->value().toInt();
        
        if (rcMin == rcMinDefault && rcMax == rcMaxDefault && rcTrim == rcTrimDefault) {
            return false;
        }
    }
    
    return true;
}

QString RadioComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

QStringList RadioComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList triggers;
    
    // The best we can do to detect the need for a radio calibration is look for trim/min/max still being
    // at defaults. We also look for attitude controls to be mapped. But since they default to channels
    // they are not a very reliable source.
    
    // Attitude control mapping is always a trigger
    triggers << "RC_MAP_ROLL" << "RC_MAP_PITCH" << "RC_MAP_YAW" << "RC_MAP_THROTTLE";
    
    // We also trigger on min/max/trim for channels 1-4 which would normally be the attitude
    // control channels. This may not always be the case, but it's the best we can
    triggers << "RC1_MIN" << "RC1_MAX" << "RC1_TRIM";
    triggers << "RC2_MIN" << "RC2_MAX" << "RC2_TRIM";
    triggers << "RC3_MIN" << "RC3_MAX" << "RC3_TRIM";
    triggers << "RC4_MIN" << "RC4_MAX" << "RC4_TRIM";
    
    return triggers;
}

QStringList RadioComponent::paramFilterList(void) const
{
    QStringList list;
    
    list << "RC*";
    
    return list;
}

QUrl RadioComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/RadioComponent.qml");
}

QUrl RadioComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/RadioComponentSummary.qml");
}

QString RadioComponent::prerequisiteSetup(void) const
{
    PX4AutoPilotPlugin* plugin = dynamic_cast<PX4AutoPilotPlugin*>(_autopilot);
    Q_ASSERT(plugin);
    
    if (!plugin->airframeComponent()->setupComplete()) {
        return plugin->airframeComponent()->name();
    }
    
    return QString();
}
