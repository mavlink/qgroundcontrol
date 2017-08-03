/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "YuneecSafetyComponent.h"
#include "PX4AutoPilotPlugin.h"

YuneecSafetyComponent::YuneecSafetyComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : SafetyComponent(vehicle, autopilot, parent)
{
}

QString
YuneecSafetyComponent::description(void) const
{
    return tr("Safety Setup is used to setup triggers for Return to Land as well as the settings for Return to Land itself.");
}

QUrl
YuneecSafetyComponent::setupSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/YuneecSafetyComponent.qml");
}

QUrl
YuneecSafetyComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput("qrc:/typhoonh/YuneecSafetyComponentSummary.qml");
}
