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
#include "PX4RCCalibration.h"
#include "PX4AutoPilotPlugin.h"

/// @brief Parameters which signal a change in setupComplete state
static const char* triggerParams[] = { "RC_MAP_MODE_SW", NULL };

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
    return "RadioComponentIcon.png";
}

bool RadioComponent::requiresSetup(void) const
{
    return true;
}

bool RadioComponent::setupComplete(void) const
{
    QVariant value;
    if (_paramMgr->getParameterValue(_paramMgr->getDefaultComponentId(), triggerParams[0], value)) {
        return value.toInt() != 0;
    } else {
        Q_ASSERT(false);
        return false;
    }
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

const char** RadioComponent::setupCompleteChangedTriggerList(void) const
{
    return triggerParams;
}

QStringList RadioComponent::paramFilterList(void) const
{
    QStringList list;
    
    list << "RC*";
    
    return list;
}

QWidget* RadioComponent::setupWidget(void) const
{
    return new PX4RCCalibration;
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
