/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "ESP8266Component.h"
#include "AutoPilotPlugin.h"

ESP8266Component::ESP8266Component(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("WiFi Bridge"))
{

}

QString ESP8266Component::name(void) const
{
    return _name;
}

QString ESP8266Component::description(void) const
{
    return tr("The ESP8266 WiFi Bridge Component is used to setup the WiFi link.");
}

QString ESP8266Component::iconResource(void) const
{
    return "/qmlimages/wifi.svg";
}

bool ESP8266Component::requiresSetup(void) const
{
    return false;
}

bool ESP8266Component::setupComplete(void) const
{
    return true;
}

QStringList ESP8266Component::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl ESP8266Component::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/ESP8266Component.qml");
}

QUrl ESP8266Component::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/qml/ESP8266ComponentSummary.qml");
}
