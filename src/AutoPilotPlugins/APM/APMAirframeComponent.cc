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
    //: Not the correct one, but it works for the moment.
    return _autopilot->getParameterFact(FactSystem::defaultComponentId, "FRAME")->rawValue().toInt() != -1;
}

QString APMAirframeComponent::setupStateDescription(void) const
{
    return QString(requiresSetup() ? "Requires calibration" : "Calibrated");
}

QStringList APMAirframeComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QStringList APMAirframeComponent::paramFilterList(void) const
{
    return QStringList();
}

QUrl APMAirframeComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMAirframeComponent.qml");
}

QUrl APMAirframeComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/APMAirframeComponentSummary.qml");
}

QString APMAirframeComponent::prerequisiteSetup(void) const
{
    return QString();
}
