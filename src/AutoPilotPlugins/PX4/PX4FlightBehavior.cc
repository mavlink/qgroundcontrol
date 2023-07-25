/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "PX4FlightBehavior.h"
#include "PX4AutoPilotPlugin.h"
#include "AirframeComponent.h"

PX4FlightBehavior::PX4FlightBehavior(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, parent)
    , _name(tr("Flight Behavior"))
{
}

QString PX4FlightBehavior::name() const
{
    return _name;
}

QString PX4FlightBehavior::description() const
{
    return tr("Flight Behavior is used to configure flight characteristics.");
}

QString PX4FlightBehavior::iconResource() const
{
    return "/qmlimages/TuningComponentIcon.png";
}

bool PX4FlightBehavior::requiresSetup() const
{
    return false;
}

bool PX4FlightBehavior::setupComplete() const
{
    return true;
}

QStringList PX4FlightBehavior::setupCompleteChangedTriggerList() const
{
    return QStringList();
}

QUrl PX4FlightBehavior::setupSource() const
{
    QString qmlFile;

    switch (_vehicle->vehicleType()) {
        case MAV_TYPE_FIXED_WING:
            qmlFile = "";
            break;
        case MAV_TYPE_QUADROTOR:
        case MAV_TYPE_COAXIAL:
        case MAV_TYPE_HELICOPTER:
        case MAV_TYPE_HEXAROTOR:
        case MAV_TYPE_OCTOROTOR:
        case MAV_TYPE_TRICOPTER:
            qmlFile = "qrc:/qml/PX4FlightBehaviorCopter.qml";
            break;
        case MAV_TYPE_VTOL_DUOROTOR:
        case MAV_TYPE_VTOL_QUADROTOR:
        case MAV_TYPE_VTOL_TILTROTOR:
        case MAV_TYPE_VTOL_RESERVED2:
        case MAV_TYPE_VTOL_RESERVED3:
        case MAV_TYPE_VTOL_RESERVED4:
        case MAV_TYPE_VTOL_RESERVED5:
            qmlFile = "";
            break;
        default:
            break;
    }

    return QUrl::fromUserInput(qmlFile);
}

QUrl PX4FlightBehavior::summaryQmlSource() const
{
    return QUrl();
}
