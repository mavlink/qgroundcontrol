/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "VideoStreamingComponent.h"

VideoStreamingComponent::VideoStreamingComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name("Video Streaming")
{
}

QString VideoStreamingComponent::name(void) const
{
    return _name;
}

QString VideoStreamingComponent::description(void) const
{
    return tr("Video Streaming Setup is used to set up connection and video quality settings.");
}

QString VideoStreamingComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/VideoComponentIcon.png");
}

bool VideoStreamingComponent::requiresSetup(void) const
{
    return false;
}

bool VideoStreamingComponent::setupComplete(void) const
{
    return true;
}

QStringList VideoStreamingComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl VideoStreamingComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/VideoStreamingComponent.qml"));
}

QUrl VideoStreamingComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/VideoStreamingComponentSummary.qml"));
}
