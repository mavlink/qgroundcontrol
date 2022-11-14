/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FrameComponent.h"

#include "QGCApplication.h"

FrameComponent::FrameComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent) :
    VehicleComponent(vehicle, autopilot, parent),
    _name(tr("Frames")), _frames(*vehicle->frames())
{
//    connect(&_frames, &Frames::hasUnsetRequiredFunctionsChanged, this, [this]() { _triggerUpdated({}); });
}

QString FrameComponent::name(void) const
{
    return _name;
}

QString FrameComponent::description(void) const
{
    return "Frames UI visualizes the cascaded frame selection interface";
}

QString FrameComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/AirframeComponentIcon.png");
}

bool FrameComponent::requiresSetup(void) const
{
    return true;
}

bool FrameComponent::setupComplete(void) const
{
    return true; // TODO: Remove. Temporary
}

QStringList FrameComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl FrameComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/FrameComponent.qml"));
}

QUrl FrameComponent::summaryQmlSource(void) const
{
    return QUrl();
}
