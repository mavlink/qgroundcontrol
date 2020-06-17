/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMTuningComponent.h"
#include "APMAutoPilotPlugin.h"
#include "APMAirframeComponent.h"
#include "ParameterManager.h"

APMTuningComponent::APMTuningComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Tuning"))
{
}

QString APMTuningComponent::name(void) const
{
    return _name;
}

QString APMTuningComponent::description(void) const
{
    return tr("Tuning Setup is used to tune the flight characteristics of the Vehicle.");
}

QString APMTuningComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/TuningComponentIcon.png");
}

bool APMTuningComponent::requiresSetup(void) const
{
    return false;
}

bool APMTuningComponent::setupComplete(void) const
{
    return true;
}

QStringList APMTuningComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl APMTuningComponent::setupSource(void) const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = QStringLiteral("qrc:/qml/APMTuningComponentCopter.qml");
            break;
        case MAV_TYPE_SUBMARINE:
            qmlFile = QStringLiteral("qrc:/qml/APMTuningComponentSub.qml");
            break;
        default:
            // No tuning panel
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl APMTuningComponent::summaryQmlSource(void) const
{
    return QUrl();
}
