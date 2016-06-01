/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "APMAirframeComponent.h"
#include "ArduCopterFirmwarePlugin.h"

APMAirframeComponent::APMAirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _requiresFrameSetup(false)
    , _name("Airframe")
{
    if (qobject_cast<ArduCopterFirmwarePlugin*>(_vehicle->firmwarePlugin()) != NULL) {
        _requiresFrameSetup = true;
        MAV_TYPE vehicleType = vehicle->vehicleType();
        if (vehicleType == MAV_TYPE_TRICOPTER || vehicleType == MAV_TYPE_HELICOPTER) {
            _requiresFrameSetup = false;
        }
    }
}

QString APMAirframeComponent::name(void) const
{
    return _name;
}

QString APMAirframeComponent::description(void) const
{
    return tr("The Airframe Component is used to select the airframe which matches your vehicle. "
              "This will in turn set up the various tuning values for flight parameters.");
}

QString APMAirframeComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/AirframeComponentIcon.png");
}

bool APMAirframeComponent::requiresSetup(void) const
{
    return _requiresFrameSetup;
}

bool APMAirframeComponent::setupComplete(void) const
{
    if (_requiresFrameSetup) {
        return _autopilot->getParameterFact(FactSystem::defaultComponentId, QStringLiteral("FRAME"))->rawValue().toInt() >= 0;
    } else {
        return true;
    }
}

QStringList APMAirframeComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;

    if (_requiresFrameSetup) {
        list << QStringLiteral("FRAME");
    }

    return list;
}

QUrl APMAirframeComponent::setupSource(void) const
{
    if (_requiresFrameSetup) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMAirframeComponent.qml"));
    } else {
        return QUrl();
    }
}

QUrl APMAirframeComponent::summaryQmlSource(void) const
{
    if (_requiresFrameSetup) {
        return QUrl::fromUserInput(QStringLiteral("qrc:/qml/APMAirframeComponentSummary.qml"));
    } else {
        return QUrl();
    }
}

QString APMAirframeComponent::prerequisiteSetup(void) const
{
    return QString();
}
