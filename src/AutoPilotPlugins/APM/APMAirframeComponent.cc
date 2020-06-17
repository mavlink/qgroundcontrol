/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMAirframeComponent.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ParameterManager.h"

const char* APMAirframeComponent::_frameClassParam = "FRAME_CLASS";

APMAirframeComponent::APMAirframeComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent      (vehicle, autopilot, parent)
    , _requiresFrameSetup   (false)
    , _name                 (tr("Frame"))
{
    ParameterManager* paramMgr = vehicle->parameterManager();

    if (paramMgr->parameterExists(FactSystem::defaultComponentId, _frameClassParam)) {
        _frameClassFact = paramMgr->getParameter(FactSystem::defaultComponentId, _frameClassParam);
        if (vehicle->vehicleType() != MAV_TYPE_HELICOPTER) {
            _requiresFrameSetup = true;
        }
    } else {
        _frameClassFact = nullptr;
    }
}

QString APMAirframeComponent::name(void) const
{
    return _name;
}

QString APMAirframeComponent::description(void) const
{
    return tr("Frame Setup is used to select the airframe which matches your vehicle.");
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
        return _frameClassFact->rawValue().toInt() != 0;
    } else {
        return true;
    }
}

QStringList APMAirframeComponent::setupCompleteChangedTriggerList(void) const
{
    QStringList list;

    if (_requiresFrameSetup) {
        list << _frameClassParam;
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
