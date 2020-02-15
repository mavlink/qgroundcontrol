/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QStringList>
#include <QDebug>

#include "TakeoffMissionItem.h"
#include "FirmwarePluginManager.h"
#include "QGCApplication.h"
#include "JsonHelper.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"

TakeoffMissionItem::TakeoffMissionItem(Vehicle* vehicle, bool flyView, MissionSettingsItem* settingsItem, QObject* parent)
    : SimpleMissionItem (vehicle, flyView, parent)
    , _settingsItem     (settingsItem)
{
    _init();
}

TakeoffMissionItem::TakeoffMissionItem(MAV_CMD takeoffCmd, Vehicle* vehicle, bool flyView, MissionSettingsItem* settingsItem, QObject* parent)
    : SimpleMissionItem (vehicle, flyView, parent)
    , _settingsItem     (settingsItem)
{
    setCommand(takeoffCmd);
    _init();
}

TakeoffMissionItem::TakeoffMissionItem(const MissionItem& missionItem, Vehicle* vehicle, bool flyView, MissionSettingsItem* settingsItem, QObject* parent)
    : SimpleMissionItem (vehicle, flyView, missionItem, parent)
    , _settingsItem     (settingsItem)
{
    _init();
}

TakeoffMissionItem::~TakeoffMissionItem()
{

}

void TakeoffMissionItem::_init(void)
{
    _editorQml = QStringLiteral("qrc:/qml/SimpleItemEditor.qml");

    connect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &TakeoffMissionItem::launchCoordinateChanged);

    if (_flyView) {
        _initLaunchTakeoffAtSameLocation();
        return;
    }

    QGeoCoordinate homePosition = _settingsItem->coordinate();
    if (!homePosition.isValid()) {
        Vehicle* activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        if (activeVehicle) {
            homePosition = activeVehicle->homePosition();
            if (homePosition.isValid()) {
                _settingsItem->setCoordinate(homePosition);
            }
        }
    }

    _initLaunchTakeoffAtSameLocation();

    if (homePosition.isValid() && coordinate().isValid()) {
        // Item already full specified, most likely from mission load from storage
        _wizardMode = false;
    } else {
        if (_launchTakeoffAtSameLocation && homePosition.isValid()) {
            _wizardMode = false;
            SimpleMissionItem::setCoordinate(homePosition);
        } else {
            _wizardMode = true;
        }
    }

    setDirty(false);
}

void TakeoffMissionItem::setLaunchTakeoffAtSameLocation(bool launchTakeoffAtSameLocation)
{
    if (launchTakeoffAtSameLocation != _launchTakeoffAtSameLocation) {
        _launchTakeoffAtSameLocation = launchTakeoffAtSameLocation;
        if (_launchTakeoffAtSameLocation) {
            setLaunchCoordinate(coordinate());
        }
        emit launchTakeoffAtSameLocationChanged(_launchTakeoffAtSameLocation);
        setDirty(true);
    }
}

void TakeoffMissionItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (coordinate != this->coordinate()) {
        SimpleMissionItem::setCoordinate(coordinate);
        if (_launchTakeoffAtSameLocation) {
            _settingsItem->setCoordinate(coordinate);
        }
    }
}

bool TakeoffMissionItem::isTakeoffCommand(MAV_CMD command)
{
    return command == MAV_CMD_NAV_TAKEOFF || command == MAV_CMD_NAV_VTOL_TAKEOFF;
}

void TakeoffMissionItem::_initLaunchTakeoffAtSameLocation(void)
{
    if (specifiesCoordinate()) {
        if (_vehicle->fixedWing()) {
            setLaunchTakeoffAtSameLocation(false);
        } else {
            // PX4 specifies a coordinate for takeoff even for non fixed wing. But it makes more sense to not have a coordinate
            // from and end user standpoint. So even for PX4 we try to keep launch and takeoff at the same position. Unless the
            // user has moved/loaded launch at a different location than takeoff.
            if (coordinate().isValid() && _settingsItem->coordinate().isValid()) {
                setLaunchTakeoffAtSameLocation(coordinate().latitude() == _settingsItem->coordinate().latitude() && coordinate().longitude() == _settingsItem->coordinate().longitude());
            } else {
                setLaunchTakeoffAtSameLocation(true);
            }

        }
    } else {
        setLaunchTakeoffAtSameLocation(true);
    }
}

bool TakeoffMissionItem::load(QTextStream &loadStream)
{
    bool success = SimpleMissionItem::load(loadStream);
    if (success) {
        _initLaunchTakeoffAtSameLocation();
    }
    return success;
}

bool TakeoffMissionItem::load(const QJsonObject& json, int sequenceNumber, QString& errorString)
{
    bool success = SimpleMissionItem::load(json, sequenceNumber, errorString);
    if (success) {
        _initLaunchTakeoffAtSameLocation();
    }
    return success;
}

void TakeoffMissionItem::setLaunchCoordinate(const QGeoCoordinate& launchCoordinate)
{
    if (!launchCoordinate.isValid()) {
        return;
    }

    _settingsItem->setCoordinate(launchCoordinate);

    if (!coordinate().isValid()) {
        QGeoCoordinate takeoffCoordinate;
        if (_launchTakeoffAtSameLocation) {
            takeoffCoordinate = launchCoordinate;
        } else {
            double altitude = this->altitude()->rawValue().toDouble();
            double distance = 0.0;

            if (coordinateHasRelativeAltitude()) {
                // Offset for fixed wing climb out of 30 degrees
                if (altitude != 0.0) {
                    distance = altitude / tan(qDegreesToRadians(30.0));
                }
            } else {
                distance = altitude * 1.5;
            }
            takeoffCoordinate = launchCoordinate.atDistanceAndAzimuth(distance, 0);
        }
        SimpleMissionItem::setCoordinate(takeoffCoordinate);
    }
}
