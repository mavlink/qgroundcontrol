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

#include "APMAirframeComponent.h"
#include "QGCQmlWidgetHolder.h"

APMAirframeComponent::APMAirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    APMComponent(vehicle, autopilot, parent),
    _name(tr("Airframe"))
{

}

QString APMAirframeComponent::name(void) const
{
    return _name;
}

QString APMAirframeComponent::description(void) const
{
    return tr("The Airframe Component is used to select the airframe which matches your vehicle. "
              "This will in turn set up the various tuning values for flight paramters.");
}

QString APMAirframeComponent::iconResource(void) const
{
    return "/qmlimages/AirframeComponentIcon.png";
}

bool APMAirframeComponent::requiresSetup(void) const
{
    return true;
}

bool APMAirframeComponent::setupComplete(void) const
{
    // You'll need to figure out which parameters trigger setup complete
#if 0
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, "SYS_AUTOSTART")->value().toInt() != 0;
#else
    return true;
#endif
}

QString APMAirframeComponent::setupStateDescription(void) const
{
    const char* stateDescription;
    
    if (requiresSetup()) {
        stateDescription = "Requires calibration";
    } else {
        stateDescription = "Calibrated";
    }
    return QString(stateDescription);
}

QStringList APMAirframeComponent::setupCompleteChangedTriggerList(void) const
{
    // You'll need to figure out which parameters trigger setup complete
#if 0
    return QStringList("SYS_AUTOSTART");
#else
    return QStringList();
#endif
}

QStringList APMAirframeComponent::paramFilterList(void) const
{
#if 0
    QStringList list;
    
    list << "SYS_AUTOSTART";
    
    return list;
#else
    return QStringList();
#endif
}

QUrl APMAirframeComponent::setupSource(void) const
{
    qDebug() << "Returning the right airframe component";
    return QUrl::fromUserInput("qrc:/qml/APMAirframeComponent.qml");
}

QUrl APMAirframeComponent::summaryQmlSource(void) const
{
    qDebug() << "Returning the right airframe component summary";
    return QUrl::fromUserInput("qrc:/qml/APMAirframeComponentSummary.qml");
}

QString APMAirframeComponent::prerequisiteSetup(void) const
{
    return QString();
}
