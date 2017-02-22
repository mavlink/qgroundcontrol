/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMVideoStreamingComponent.h"

APMVideoStreamingComponent::APMVideoStreamingComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name("Video Streaming")
{
}

QString APMVideoStreamingComponent::name(void) const
{
    return _name;
}

QString APMVideoStreamingComponent::description(void) const
{
    return tr("Video Streaming Setup is used to set up connection and video quality settings.");
}

QString APMVideoStreamingComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/VideoComponentIcon.png");
}

bool APMVideoStreamingComponent::requiresSetup(void) const
{
    return false;
}

bool APMVideoStreamingComponent::setupComplete(void) const
{
    return true;
}

QStringList APMVideoStreamingComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMVideoStreamingComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMVideoStreamingComponent.qml"));
}

QUrl APMVideoStreamingComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMVideoStreamingComponentSummary.qml"));
}

QString APMVideoStreamingComponent::prerequisiteSetup(void) const
{
    return QString();
}
