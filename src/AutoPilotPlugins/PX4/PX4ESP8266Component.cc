/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include "PX4ESP8266Component.h"
#include "PX4AutoPilotPlugin.h"

PX4ESP8266Component::PX4ESP8266Component(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : PX4Component(vehicle, autopilot, parent)
    , _name(tr("WiFi Bridge"))
{

}

QString PX4ESP8266Component::name(void) const
{
    return _name;
}

QString PX4ESP8266Component::description(void) const
{
    return tr("The ESP8266 WiFi Bridge Component is used to setup the WiFi link.");
}

QString PX4ESP8266Component::iconResource(void) const
{
    return "/qmlimages/wifi.svg";
}

bool PX4ESP8266Component::requiresSetup(void) const
{
    return false;
}

bool PX4ESP8266Component::setupComplete(void) const
{
    return true;
}

QStringList PX4ESP8266Component::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl PX4ESP8266Component::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/ESP8266Component.qml");
}

QUrl PX4ESP8266Component::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/ESP8266ComponentSummary.qml");
}

QString PX4ESP8266Component::prerequisiteSetup(void) const
{
    return QString();
}
