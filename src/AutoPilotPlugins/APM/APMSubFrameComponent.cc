/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>
///     @author Jacob Walser <jwalser90@gmail.com>

#include "APMSubFrameComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"

APMSubFrameComponent::APMSubFrameComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Frame"))
{
}

QString APMSubFrameComponent::name(void) const
{
    return _name;
}

QString APMSubFrameComponent::description(void) const
{
    return tr("Frame setup allows you to choose your vehicle's motor configuration. Install <b>clockwise</b>" \
              "<br>propellers on the <b>green thrusters</b> and <b>counter-clockwise</b> propellers on the <b>blue thrusters</b>" \
              "<br>(or vice-versa). The flight controller will need to be rebooted to apply changes." \
              "<br>When selecting a frame, you can choose to load the default parameter set for that frame configuration if available.");
}

QString APMSubFrameComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/SubFrameComponentIcon.png");
}

bool APMSubFrameComponent::requiresSetup(void) const
{
    return false;
}

bool APMSubFrameComponent::setupComplete(void) const
{
    return true;
}

QStringList APMSubFrameComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMSubFrameComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMSubFrameComponent.qml"));
}

QUrl APMSubFrameComponent::summaryQmlSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMSubFrameComponentSummary.qml"));
}
