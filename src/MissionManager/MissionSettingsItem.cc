/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionSettingsItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "SimpleMissionItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "MissionCommandUIInfo.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(MissionSettingsComplexItemLog, "MissionSettingsComplexItemLog")

const char* MissionSettingsItem::jsonComplexItemTypeValue = "MissionSettings";

const char* MissionSettingsItem::_plannedHomePositionAltitudeName =  "PlannedHomePositionAltitude";
const char* MissionSettingsItem::_missionFlightSpeedName =           "FlightSpeed";
const char* MissionSettingsItem::_missionEndActionName =             "MissionEndAction";

QMap<QString, FactMetaData*> MissionSettingsItem::_metaDataMap;

MissionSettingsItem::MissionSettingsItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem(vehicle, parent)
    , _specifyMissionFlightSpeed(false)
    , _plannedHomePositionAltitudeFact  (0, _plannedHomePositionAltitudeName,   FactMetaData::valueTypeDouble)
    , _missionFlightSpeedFact           (0, _missionFlightSpeedName,            FactMetaData::valueTypeDouble)
    , _missionEndActionFact             (0, _missionEndActionName,              FactMetaData::valueTypeUint32)
    , _sequenceNumber(0)
    , _dirty(false)
{
    _editorQml = "qrc:/qml/MissionSettingsEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/MissionSettings.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _plannedHomePositionAltitudeFact.setMetaData    (_metaDataMap[_plannedHomePositionAltitudeName]);
    _missionFlightSpeedFact.setMetaData             (_metaDataMap[_missionFlightSpeedName]);
    _missionEndActionFact.setMetaData               (_metaDataMap[_missionEndActionName]);

    _plannedHomePositionAltitudeFact.setRawValue    (_plannedHomePositionAltitudeFact.rawDefaultValue());
    _missionEndActionFact.setRawValue               (_missionEndActionFact.rawDefaultValue());

    // FIXME: Flight speed default value correctly based firmware parameter if online
    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    Fact* speedFact = vehicle->multiRotor() ? appSettings->offlineEditingHoverSpeed() : appSettings->offlineEditingCruiseSpeed();
    _missionFlightSpeedFact.setRawValue(speedFact->rawValue().toDouble());

    setHomePositionSpecialCase(true);

    connect(this,               &MissionSettingsItem::specifyMissionFlightSpeedChanged,  this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(&_cameraSection,    &CameraSection::missionItemCountChanged,                        this, &MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber);

    connect(&_plannedHomePositionAltitudeFact,  &Fact::valueChanged, this, &MissionSettingsItem::_setDirty);
    connect(&_plannedHomePositionAltitudeFact,  &Fact::valueChanged, this, &MissionSettingsItem::_updateAltitudeInCoordinate);

    connect(&_missionFlightSpeedFact,           &Fact::valueChanged, this, &MissionSettingsItem::_setDirty);
    connect(&_missionEndActionFact,             &Fact::valueChanged, this, &MissionSettingsItem::_setDirty);

    connect(&_cameraSection, &CameraSection::dirtyChanged, this, &MissionSettingsItem::_cameraSectionDirtyChanged);

    connect(&_missionFlightSpeedFact,   &Fact::valueChanged,                        this, &MissionSettingsItem::specifiedFlightSpeedChanged);
    connect(&_cameraSection,            &CameraSection::specifyGimbalChanged,       this, &MissionSettingsItem::specifiedGimbalYawChanged);
    connect(&_cameraSection,            &CameraSection::specifiedGimbalYawChanged,  this, &MissionSettingsItem::specifiedGimbalYawChanged);
}


void MissionSettingsItem::setSpecifyMissionFlightSpeed(bool specifyMissionFlightSpeed)
{
    if (specifyMissionFlightSpeed != _specifyMissionFlightSpeed) {
        _specifyMissionFlightSpeed = specifyMissionFlightSpeed;
        emit specifyMissionFlightSpeedChanged(specifyMissionFlightSpeed);
    }
}

int MissionSettingsItem::lastSequenceNumber(void) const
{
    int lastSequenceNumber = _sequenceNumber;

    if (_specifyMissionFlightSpeed) {
        lastSequenceNumber++;
    }
    lastSequenceNumber += _cameraSection.missionItemCount();

    return lastSequenceNumber;
}

void MissionSettingsItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void MissionSettingsItem::save(QJsonArray&  missionItems)
{
    QList<MissionItem*> items;

    appendMissionItems(items, this);

    // First item show be planned home position, we are not reponsible for save/load
    // Remaining items we just output as is
    for (int i=1; i<items.count(); i++) {
        MissionItem* item = items[i];
        QJsonObject saveObject;
        item->save(saveObject);
        missionItems.append(saveObject);
        item->deleteLater();
    }
}

void MissionSettingsItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool MissionSettingsItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    Q_UNUSED(complexObject);
    Q_UNUSED(sequenceNumber);
    Q_UNUSED(errorString);

    return true;
}

double MissionSettingsItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    Q_UNUSED(other);
    return 0;
}

bool MissionSettingsItem::specifiesCoordinate(void) const
{
    return false;
}

void MissionSettingsItem::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent)
{
    int seqNum = _sequenceNumber;

    // IMPORTANT NOTE: If anything changes here you must also change MissionSettingsItem::scanForMissionSettings

    // Planned home position
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        MAV_FRAME_GLOBAL,
                                        0,                      // Hold time
                                        0,                      // Acceptance radius
                                        0,                      // Not sure?
                                        0,                      // Yaw
                                        coordinate().latitude(),
                                        coordinate().longitude(),
                                        _plannedHomePositionAltitudeFact.rawValue().toDouble(),
                                        true,                   // autoContinue
                                        false,                  // isCurrentItem
                                        missionItemParent);
    items.append(item);

    if (_specifyMissionFlightSpeed) {
        qCDebug(MissionSettingsComplexItemLog) << "Appending MAV_CMD_DO_CHANGE_SPEED";
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_CHANGE_SPEED,
                                            MAV_FRAME_MISSION,
                                            _vehicle->multiRotor() ? 1 /* groundspeed */ : 0 /* airspeed */,    // Change airspeed or groundspeed
                                            _missionFlightSpeedFact.rawValue().toDouble(),
                                            -1,                                                                 // No throttle change
                                            0,                                                                  // Absolute speed change
                                            0, 0, 0,                                                            // param 5-7 not used
                                            true,                                                               // autoContinue
                                            false,                                                              // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }

    _cameraSection.appendMissionItems(items, missionItemParent, seqNum);
}

bool MissionSettingsItem::addMissionEndAction(QList<MissionItem*>& items, int seqNum, QObject* missionItemParent)
{
    MissionItem* item = NULL;

    // IMPORTANT NOTE: If anything changes here you must also change MissionSettingsItem::scanForMissionSettings

    // Find last waypoint coordinate information so we have a lat/lon/alt to use
    QGeoCoordinate  lastWaypointCoord;
    MAV_FRAME       lastWaypointFrame;

    bool found = false;
    for (int i=items.count()-1; i>0; i--) {
        MissionItem* missionItem = items[i];

        const MissionCommandUIInfo* uiInfo = qgcApp()->toolbox()->missionCommandTree()->getUIInfo(_vehicle, (MAV_CMD)missionItem->command());
        if (uiInfo->specifiesCoordinate() && !uiInfo->isStandaloneCoordinate()) {
            lastWaypointCoord = missionItem->coordinate();
            lastWaypointFrame = missionItem->frame();
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }

    switch(_missionEndActionFact.rawValue().toInt()) {
    case MissionEndLoiter:
        qCDebug(MissionSettingsComplexItemLog) << "Appending end action Loiter seqNum" << seqNum;
        item = new MissionItem(seqNum,
                               MAV_CMD_NAV_LOITER_UNLIM,
                               lastWaypointFrame,
                               0, 0,                        // param 1-2 unused
                               0,                           // use default loiter radius
                               0,                           // param 4 not used
                               lastWaypointCoord.latitude(),
                               lastWaypointCoord.longitude(),
                               lastWaypointCoord.altitude(),
                               true,                        // autoContinue
                               false,                       // isCurrentItem
                               missionItemParent);
        break;
    case MissionEndLand:
        qCDebug(MissionSettingsComplexItemLog) << "Appending end action Land seqNum" << seqNum;
        item = new MissionItem(seqNum,
                               MAV_CMD_NAV_LAND,
                               MAV_FRAME_GLOBAL_RELATIVE_ALT,
                               0,                               // abort Altitude
                               0, 0,                            // not used
                               0,                               // yaw
                               lastWaypointCoord.latitude(),
                               lastWaypointCoord.longitude(),
                               0,                               // altitude
                               true,                            // autoContinue
                               false,                           // isCurrentItem
                               missionItemParent);
        break;
    case MissionEndRTL:
        qCDebug(MissionSettingsComplexItemLog) << "Appending end action RTL seqNum" << seqNum;
        item = new MissionItem(seqNum,
                               MAV_CMD_NAV_RETURN_TO_LAUNCH,
                               MAV_FRAME_MISSION,
                               0, 0, 0, 0, 0, 0, 0,        // param 1-7 not used
                               true,                       // autoContinue
                               false,                      // isCurrentItem
                               missionItemParent);
        break;
    default:
        break;
    }

    if (item) {
        items.append(item);
        return true;
    } else {
        return false;
    }
}

bool MissionSettingsItem::scanForMissionSettings(QmlObjectListModel* visualItems, int scanIndex, Vehicle* vehicle)
{
    bool foundSpeed = false;
    bool foundCameraSection = false;
    bool stopLooking = false;

    qCDebug(MissionSettingsComplexItemLog) << "MissionSettingsItem::scanForMissionSettings count:scanIndex" << visualItems->count() << scanIndex;

    MissionSettingsItem* settingsItem = visualItems->value<MissionSettingsItem*>(scanIndex);
    if (!settingsItem) {
        qWarning() << "Item specified by scanIndex not MissionSettingsItem";
        return false;
    }

    // Scan through the initial mission items for possible mission settings

    scanIndex++;
    while (!stopLooking && visualItems->count() > 1) {
        SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
        if (!item) {
            // We hit a complex item, there can be no more possible mission settings
            break;
        }
        MissionItem& missionItem = item->missionItem();

        qCDebug(MissionSettingsComplexItemLog) << item->command() << missionItem.param1() << missionItem.param2() << missionItem.param3() << missionItem.param4() << missionItem.param5() << missionItem.param6() << missionItem.param7() ;

        // See MissionSettingsItem::getMissionItems for specs on what compomises a known mission setting

        switch ((MAV_CMD)item->command()) {
        case MAV_CMD_DO_CHANGE_SPEED:
            if (!foundSpeed && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                if (vehicle->multiRotor()) {
                    if (missionItem.param1() != 1) {
                        stopLooking = true;
                        break;
                    }
                } else {
                    if (missionItem.param1() != 0) {
                        stopLooking = true;
                        break;
                    }
                }
                foundSpeed = true;
                settingsItem->setSpecifyMissionFlightSpeed(true);
                settingsItem->missionFlightSpeed()->setRawValue(missionItem.param2());
                visualItems->removeAt(scanIndex)->deleteLater();
                qCDebug(MissionSettingsComplexItemLog) << "Scan: Found MAV_CMD_DO_CHANGE_SPEED";
                continue;
            }
            stopLooking = true;
            break;

        default:
            if (!foundCameraSection) {
                if (settingsItem->_cameraSection.scanForCameraSection(visualItems, scanIndex)) {
                    foundCameraSection = true;
                    qCDebug(MissionSettingsComplexItemLog) << "Scan: Found Camera Section";
                    continue;
                }
            }
            stopLooking = true;
            break;
        }
    }

    // Look at the end of the mission for end actions

    int lastIndex = visualItems->count() - 1;
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(lastIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();

        switch ((MAV_CMD)item->command()) {
        case MAV_CMD_NAV_LOITER_UNLIM:
            if (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0) {
                qCDebug(MissionSettingsComplexItemLog) << "Scan: Found end action Loiter";
                settingsItem->missionEndAction()->setRawValue(MissionEndLoiter);
                visualItems->removeAt(lastIndex)->deleteLater();
            }
            break;

        case MAV_CMD_NAV_LAND:
            if (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param7() == 0) {
                qCDebug(MissionSettingsComplexItemLog) << "Scan: Found end action Land";
                settingsItem->missionEndAction()->setRawValue(MissionEndLand);
                visualItems->removeAt(lastIndex)->deleteLater();
            }
            break;

        case MAV_CMD_NAV_RETURN_TO_LAUNCH:
            if (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                qCDebug(MissionSettingsComplexItemLog) << "Scan: Found end action RTL";
                settingsItem->missionEndAction()->setRawValue(MissionEndRTL);
                visualItems->removeAt(lastIndex)->deleteLater();
            }
            break;

        default:
            break;
        }
    }

    return foundSpeed || foundCameraSection;
}

double MissionSettingsItem::complexDistance(void) const
{
    return 0;
}

void MissionSettingsItem::_setDirty(void)
{
    setDirty(true);
}

void MissionSettingsItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (_plannedHomePositionCoordinate != coordinate) {
        _plannedHomePositionCoordinate = coordinate;
        emit coordinateChanged(coordinate);
        emit exitCoordinateChanged(coordinate);
        _plannedHomePositionAltitudeFact.setRawValue(coordinate.altitude());
    }
}

void MissionSettingsItem::_setDirtyAndUpdateLastSequenceNumber(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
    setDirty(true);
}

void MissionSettingsItem::_cameraSectionDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

double MissionSettingsItem::specifiedFlightSpeed(void)
{
    return _specifyMissionFlightSpeed ? _missionFlightSpeedFact.rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

double MissionSettingsItem::specifiedGimbalYaw(void)
{
    return _cameraSection.specifyGimbal() ? _cameraSection.gimbalYaw()->rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void MissionSettingsItem::_updateAltitudeInCoordinate(QVariant value)
{
    double newAltitude = value.toDouble();

    if (!qFuzzyCompare(_plannedHomePositionCoordinate.altitude(), newAltitude)) {

        _plannedHomePositionCoordinate.setAltitude(newAltitude);
        emit coordinateChanged(_plannedHomePositionCoordinate);
        emit exitCoordinateChanged(_plannedHomePositionCoordinate);
    }
}
