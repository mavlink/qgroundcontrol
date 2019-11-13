/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    if (_settingsItem->coordinate().isValid()) {
        // Either the user has set a Launch location or it came from a connected vehicle.
        // Use it as starting point.
        setCoordinate(_settingsItem->coordinate());
    }
    connect(_settingsItem, &MissionSettingsItem::coordinateChanged, this, &TakeoffMissionItem::launchCoordinateChanged);

    _initLaunchTakeoffAtSameLocation();

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
    if (this->coordinate().isValid() || !_vehicle->fixedWing()) {
        if (coordinate != this->coordinate()) {
            if (_launchTakeoffAtSameLocation) {
                setLaunchCoordinate(coordinate);
            }
            SimpleMissionItem::setCoordinate(coordinate);
        }
    } else {
        // First time setup for fixed wing
        if (!launchCoordinate().isValid()) {
            setLaunchCoordinate(coordinate);
        }
        SimpleMissionItem::setCoordinate(launchCoordinate().atDistanceAndAzimuth(60, 0));
    }
}

bool TakeoffMissionItem::isTakeoffCommand(MAV_CMD command)
{
    return command == MAV_CMD_NAV_TAKEOFF || command == MAV_CMD_NAV_VTOL_TAKEOFF;
}

void TakeoffMissionItem::_initLaunchTakeoffAtSameLocation(void)
{
    if (_vehicle->fixedWing()) {
        setLaunchTakeoffAtSameLocation(!specifiesCoordinate());
    } else {
        setLaunchTakeoffAtSameLocation(coordinate().latitude() == _settingsItem->coordinate().latitude() && coordinate().longitude() == _settingsItem->coordinate().longitude());
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
