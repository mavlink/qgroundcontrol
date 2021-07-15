/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FMTTuningComponent.h"
#include "FMTAutoPilotPlugin.h"
#include "AirframeComponent.h"

FMTTuningComponent::FMTTuningComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Tuning"))
{
}

QString FMTTuningComponent::name(void) const
{
    return _name;
}

QString FMTTuningComponent::description(void) const
{
    return tr("Tuning Setup is used to tune the flight characteristics of the Vehicle.");
}

QString FMTTuningComponent::iconResource(void) const
{
    return "/qmlimages/TuningComponentIcon.png";
}

bool FMTTuningComponent::requiresSetup(void) const
{
    return false;
}

bool FMTTuningComponent::setupComplete(void) const
{
    return true;
}

QStringList FMTTuningComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl FMTTuningComponent::setupSource(void) const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
            qmlFile = "qrc:/qml/FMTTuningComponentPlane.qml";
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = "qrc:/qml/FMTTuningComponentCopter.qml";
            break;
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
        case MAV_TYPE_VTOL_RESERVED5:
            qmlFile = "qrc:/qml/FMTTuningComponentVTOL.qml";
            break;
        default:
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl FMTTuningComponent::summaryQmlSource(void) const
{
    return QUrl();
}
