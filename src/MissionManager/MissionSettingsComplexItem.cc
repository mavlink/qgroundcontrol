/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionSettingsComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGroundControlQmlGlobal.h"
#include "SimpleMissionItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QPolygonF>

QGC_LOGGING_CATEGORY(MissionSettingsComplexItemLog, "MissionSettingsComplexItemLog")

const char* MissionSettingsComplexItem::jsonComplexItemTypeValue = "MissionSettings";

const char* MissionSettingsComplexItem::_plannedHomePositionLatitudeName =  "PlannedHomePositionLatitude";
const char* MissionSettingsComplexItem::_plannedHomePositionLongitudeName = "PlannedHomePositionLongitude";
const char* MissionSettingsComplexItem::_plannedHomePositionAltitudeName =  "PlannedHomePositionAltitude";
const char* MissionSettingsComplexItem::_missionFlightSpeedName =           "FlightSpeed";
const char* MissionSettingsComplexItem::_gimbalPitchName =                  "GimbalPitch";
const char* MissionSettingsComplexItem::_gimbalYawName =                    "GimbalYaw";
const char* MissionSettingsComplexItem::_cameraActionName =                 "CameraAction";
const char* MissionSettingsComplexItem::_cameraPhotoIntervalDistanceName =  "CameraPhotoIntervalDistance";
const char* MissionSettingsComplexItem::_cameraPhotoIntervalTimeName =      "CameraPhotoIntervalTime";
const char* MissionSettingsComplexItem::_missionEndActionName =             "MissionEndAction";

QMap<QString, FactMetaData*> MissionSettingsComplexItem::_metaDataMap;

MissionSettingsComplexItem::MissionSettingsComplexItem(Vehicle* vehicle, QObject* parent)
    : ComplexMissionItem(vehicle, parent)
    , _specifyMissionFlightSpeed(false)
    , _specifyGimbal(false)
    , _plannedHomePositionLatitudeFact  (0, _plannedHomePositionLatitudeName,   FactMetaData::valueTypeDouble)
    , _plannedHomePositionLongitudeFact (0, _plannedHomePositionLongitudeName,  FactMetaData::valueTypeDouble)
    , _plannedHomePositionAltitudeFact  (0, _plannedHomePositionAltitudeName,   FactMetaData::valueTypeDouble)
    , _missionFlightSpeedFact           (0, _missionFlightSpeedName,            FactMetaData::valueTypeDouble)
    , _gimbalYawFact                    (0, _gimbalYawName,                     FactMetaData::valueTypeDouble)
    , _gimbalPitchFact                  (0, _gimbalPitchName,                   FactMetaData::valueTypeDouble)
    , _cameraActionFact                 (0, _cameraActionName,                  FactMetaData::valueTypeDouble)
    , _cameraPhotoIntervalDistanceFact  (0, _cameraPhotoIntervalDistanceName,   FactMetaData::valueTypeDouble)
    , _cameraPhotoIntervalTimeFact      (0, _cameraPhotoIntervalTimeName,       FactMetaData::valueTypeUint32)
    , _missionEndActionFact             (0, _missionEndActionName,              FactMetaData::valueTypeUint32)
    , _sequenceNumber(0)
    , _dirty(false)
{
    _editorQml = "qrc:/qml/MissionSettingsEditor.qml";

    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/MissionSettings.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _plannedHomePositionLatitudeFact.setMetaData    (_metaDataMap[_plannedHomePositionLatitudeName]);
    _plannedHomePositionLongitudeFact.setMetaData   (_metaDataMap[_plannedHomePositionLongitudeName]);
    _plannedHomePositionAltitudeFact.setMetaData    (_metaDataMap[_plannedHomePositionAltitudeName]);
    _missionFlightSpeedFact.setMetaData             (_metaDataMap[_missionFlightSpeedName]);
    _gimbalPitchFact.setMetaData                    (_metaDataMap[_gimbalPitchName]);
    _gimbalYawFact.setMetaData                      (_metaDataMap[_gimbalYawName]);
    _cameraActionFact.setMetaData                   (_metaDataMap[_cameraActionName]);
    _cameraPhotoIntervalDistanceFact.setMetaData    (_metaDataMap[_cameraPhotoIntervalDistanceName]);
    _cameraPhotoIntervalTimeFact.setMetaData        (_metaDataMap[_cameraPhotoIntervalTimeName]);
    _missionEndActionFact.setMetaData               (_metaDataMap[_missionEndActionName]);

    _plannedHomePositionLatitudeFact.setRawValue    (_plannedHomePositionLatitudeFact.rawDefaultValue());
    _plannedHomePositionLongitudeFact.setRawValue   (_plannedHomePositionLongitudeFact.rawDefaultValue());
    _plannedHomePositionAltitudeFact.setRawValue    (_plannedHomePositionAltitudeFact.rawDefaultValue());
    _gimbalPitchFact.setRawValue                    (_gimbalPitchFact.rawDefaultValue());
    _gimbalYawFact.setRawValue                      (_gimbalYawFact.rawDefaultValue());
    _cameraActionFact.setRawValue                   (_cameraActionFact.rawDefaultValue());
    _cameraPhotoIntervalDistanceFact.setRawValue    (_cameraPhotoIntervalDistanceFact.rawDefaultValue());
    _cameraPhotoIntervalTimeFact.setRawValue        (_cameraPhotoIntervalTimeFact.rawDefaultValue());
    _missionEndActionFact.setRawValue               (_missionEndActionFact.rawDefaultValue());

    // FIXME: Flight speed default value correctly based firmware parameter if online
    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    Fact* speedFact = vehicle->multiRotor() ? appSettings->offlineEditingHoverSpeed() : appSettings->offlineEditingCruiseSpeed();
    _missionFlightSpeedFact.setRawValue(speedFact->rawValue().toDouble());

    setHomePositionSpecialCase(true);

    connect(this, &MissionSettingsComplexItem::specifyMissionFlightSpeedChanged,    this, &MissionSettingsComplexItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(this, &MissionSettingsComplexItem::specifyGimbalChanged,                this, &MissionSettingsComplexItem::_setDirtyAndUpdateLastSequenceNumber);

    connect(&_plannedHomePositionLatitudeFact,  &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirtyAndUpdateCoordinate);
    connect(&_plannedHomePositionLongitudeFact, &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirtyAndUpdateCoordinate);
    connect(&_plannedHomePositionAltitudeFact,  &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirtyAndUpdateCoordinate);
    connect(&_missionFlightSpeedFact,           &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirty);
    connect(&_gimbalPitchFact,                  &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirty);
    connect(&_gimbalYawFact,                    &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirty);
    connect(&_cameraActionFact,                 &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirtyAndUpdateLastSequenceNumber);
    connect(&_cameraPhotoIntervalDistanceFact,  &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirty);
    connect(&_cameraPhotoIntervalTimeFact,      &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirty);
    connect(&_missionEndActionFact,             &Fact::valueChanged, this, &MissionSettingsComplexItem::_setDirty);
}


void MissionSettingsComplexItem::setSpecifyMissionFlightSpeed(bool specifyMissionFlightSpeed)
{
    if (specifyMissionFlightSpeed != _specifyMissionFlightSpeed) {
        _specifyMissionFlightSpeed = specifyMissionFlightSpeed;
        emit specifyMissionFlightSpeedChanged(specifyMissionFlightSpeed);
    }
}

void MissionSettingsComplexItem::setSpecifyGimbal(bool specifyGimbal)
{
    if (specifyGimbal != _specifyGimbal) {
        _specifyGimbal = specifyGimbal;
        emit specifyGimbalChanged(specifyGimbal);
    }
}

int MissionSettingsComplexItem::lastSequenceNumber(void) const
{
    int lastSequenceNumber = _sequenceNumber + 1;   // +1 for planned home position

    if (_specifyMissionFlightSpeed) {
        lastSequenceNumber++;
    }
    if (_specifyGimbal) {
        lastSequenceNumber++;
    }
    if (_cameraActionFact.rawValue().toInt() != CameraActionNone) {
        lastSequenceNumber++;
    }

    return lastSequenceNumber;
}

void MissionSettingsComplexItem::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void MissionSettingsComplexItem::save(QJsonArray&  missionItems) const
{
    QmlObjectListModel* items = getMissionItems();

    // First item show be planned home position, we are not reponsible for save/load
    // Remained we just output as is
    for (int i=1; i<items->count(); i++) {
        MissionItem* item = items->value<MissionItem*>(i);
        QJsonObject saveObject;
        item->save(saveObject);
        missionItems.append(saveObject);
    }

    items->deleteLater();
}

void MissionSettingsComplexItem::setSequenceNumber(int sequenceNumber)
{
    if (_sequenceNumber != sequenceNumber) {
        _sequenceNumber = sequenceNumber;
        emit sequenceNumberChanged(sequenceNumber);
        emit lastSequenceNumberChanged(lastSequenceNumber());
    }
}

bool MissionSettingsComplexItem::load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString)
{
    Q_UNUSED(complexObject);
    Q_UNUSED(sequenceNumber);
    Q_UNUSED(errorString);
#if 0
    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { JsonHelper::jsonVersionKey,                   QJsonValue::Double, true },
        { VisualMissionItem::jsonTypeKey,               QJsonValue::String, true },
        { ComplexMissionItem::jsonComplexItemTypeKey,   QJsonValue::String, true },
        { _jsonLoiterCoordinateKey,                     QJsonValue::Array,  true },
        { _jsonLoiterRadiusKey,                         QJsonValue::Double, true },
        { _jsonLoiterClockwiseKey,                      QJsonValue::Bool,   true },
        { _jsonLoiterAltitudeRelativeKey,               QJsonValue::Bool,   true },
        { _jsonLandingCoordinateKey,                    QJsonValue::Array,  true },
        { _jsonLandingAltitudeRelativeKey,              QJsonValue::Bool,   true },
    };
    if (!JsonHelper::validateKeys(complexObject, keyInfoList, errorString)) {
        return false;
    }

    QString itemType = complexObject[VisualMissionItem::jsonTypeKey].toString();
    QString complexType = complexObject[ComplexMissionItem::jsonComplexItemTypeKey].toString();
    if (itemType != VisualMissionItem::jsonTypeComplexItemValue || complexType != jsonComplexItemTypeValue) {
        errorString = tr("QGroundControl does not support loading this complex mission item type: %1:2").arg(itemType).arg(complexType);
        return false;
    }

    setSequenceNumber(sequenceNumber);

    QGeoCoordinate coordinate;
    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLoiterCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _loiterCoordinate = coordinate;
    _loiterAltitudeFact.setRawValue(coordinate.altitude());

    if (!JsonHelper::loadGeoCoordinate(complexObject[_jsonLandingCoordinateKey], true /* altitudeRequired */, coordinate, errorString)) {
        return false;
    }
    _landingCoordinate = coordinate;
    _landingAltitudeFact.setRawValue(coordinate.altitude());

    _loiterRadiusFact.setRawValue(complexObject[_jsonLoiterRadiusKey].toDouble());
    _loiterClockwise  = complexObject[_jsonLoiterClockwiseKey].toBool();
    _loiterAltitudeRelative = complexObject[_jsonLoiterAltitudeRelativeKey].toBool();
    _landingAltitudeRelative = complexObject[_jsonLandingAltitudeRelativeKey].toBool();

    _landingCoordSet = true;
    _recalcFromHeadingAndDistanceChange();
#endif
    return true;
}

double MissionSettingsComplexItem::greatestDistanceTo(const QGeoCoordinate &other) const
{
    Q_UNUSED(other);
    return 0;
}

bool MissionSettingsComplexItem::specifiesCoordinate(void) const
{
    return false;
}

QmlObjectListModel* MissionSettingsComplexItem::getMissionItems(void) const
{
    QmlObjectListModel* pMissionItems = new QmlObjectListModel;

    int seqNum = _sequenceNumber;

    // IMPORTANT NOTE: If anything changed here you just also change MissionSettingsComplexItem::scanForMissionSettings

    // Planned home position
    MissionItem* item = new MissionItem(seqNum++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        MAV_FRAME_GLOBAL,
                                        0,                      // Hold time
                                        0,                      // Acceptance radius
                                        0,                      // Not sure?
                                        0,                      // Yaw
                                        _plannedHomePositionLatitudeFact.rawValue().toDouble(),
                                        _plannedHomePositionLongitudeFact.rawValue().toDouble(),
                                        _plannedHomePositionAltitudeFact.rawValue().toDouble(),
                                        true,                   // autoContinue
                                        false,                  // isCurrentItem
                                        pMissionItems);         // parent - allow delete on pMissionItems to delete everthing
    pMissionItems->append(item);

    if (_specifyGimbal) {
        MissionItem* item = new MissionItem(seqNum++,
                                            MAV_CMD_DO_MOUNT_CONTROL,
                                            MAV_FRAME_MISSION,
                                            _gimbalPitchFact.rawValue().toDouble(),
                                            0,                                      // Gimbal roll
                                            _gimbalYawFact.rawValue().toDouble(),
                                            0, 0, 0,                                // param 4-6 not used
                                            MAV_MOUNT_MODE_MAVLINK_TARGETING,
                                            true,                                   // autoContinue
                                            false,                                  // isCurrentItem
                                            pMissionItems);                         // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }

    if (_specifyMissionFlightSpeed) {
        qDebug() << _missionFlightSpeedFact.rawValue().toDouble();
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
                                            pMissionItems);                                                     // parent - allow delete on pMissionItems to delete everthing
        pMissionItems->append(item);
    }

    if (_cameraActionFact.rawValue().toInt() != CameraActionNone) {
        MissionItem* item = NULL;

        switch (_cameraActionFact.rawValue().toInt()) {
        case TakePhotosIntervalTime:
            item = new MissionItem(seqNum++,
                                   MAV_CMD_IMAGE_START_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   _cameraPhotoIntervalTimeFact.rawValue().toInt(), // Interval
                                   0,                                               // Unlimited photo count
                                   -1,                                              // Max resolution
                                   0, 0,                                            // param 4-5 not used
                                   0,                                               // Camera ID
                                   0,                                               // param 7 not used
                                   true,                                            // autoContinue
                                   false,                                           // isCurrentItem
                                   pMissionItems);                                  // parent - allow delete on pMissionItems to delete everthing
            break;
        case TakePhotoIntervalDistance:
            item = new MissionItem(seqNum++,
                                   MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                   MAV_FRAME_MISSION,
                                   _cameraPhotoIntervalDistanceFact.rawValue().toDouble(),  // Trigger distance
                                   0, 0, 0, 0, 0, 0,                                        // param 2-7 not used
                                   true,                                                    // autoContinue
                                   false,                                                   // isCurrentItem
                                   pMissionItems);                                          // parent - allow delete on pMissionItems to delete everthing
            break;
        case TakeVideo:
            item = new MissionItem(seqNum++,
                                   MAV_CMD_VIDEO_START_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                                               // Camera ID
                                   -1,                                              // Max fps
                                   -1,                                              // Max resolution
                                   0, 0, 0, 0,                                      // param 5-7 not used
                                   true,                                            // autoContinue
                                   false,                                           // isCurrentItem
                                   pMissionItems);                                  // parent - allow delete on pMissionItems to delete everthing
            break;
        }
        if (item) {
            pMissionItems->append(item);
        }
    }

    return pMissionItems;
}

void MissionSettingsComplexItem::scanForMissionSettings(QmlObjectListModel* visualItems, Vehicle* vehicle)
{
    bool foundGimbal = false;
    bool foundSpeed = false;
    bool foundCameraAction = false;

    MissionSettingsComplexItem* settingsItem = visualItems->value<MissionSettingsComplexItem*>(0);
    if (!settingsItem) {
        qWarning() << "First item is not MissionSettingsComplexItem";
        return;
    }

    // Scan through the initial mission items for possible mission settings

    while (visualItems->count()> 1) {
        SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(1);
        if (!item) {
            // We hit a complex item there can be no more possible mission settings
            return;
        }
        MissionItem& missionItem = item->missionItem();

        // See MissionSettingsComplexItem::getMissionItems for specs on what compomises a known mission settings

        switch ((MAV_CMD)item->command()) {
        case MAV_CMD_DO_MOUNT_CONTROL:
            if (!foundGimbal && missionItem.param2() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == MAV_MOUNT_MODE_MAVLINK_TARGETING) {
                foundGimbal = true;
                settingsItem->setSpecifyGimbal(true);
                settingsItem->gimbalPitch()->setRawValue(missionItem.param1());
                settingsItem->gimbalYaw()->setRawValue(missionItem.param3());
                visualItems->removeAt(1)->deleteLater();
            } else {
                return;
            }
            break;

        case MAV_CMD_DO_CHANGE_SPEED:
            if (!foundSpeed && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                if (vehicle->multiRotor()) {
                    if (missionItem.param1() != 1) {
                        return;
                    }
                } else {
                    if (missionItem.param1() != 0) {
                        return;
                    }
                }
                foundSpeed = true;
                settingsItem->setSpecifyMissionFlightSpeed(true);
                settingsItem->missionFlightSpeed()->setRawValue(missionItem.param2());
                visualItems->removeAt(1)->deleteLater();
            } else {
                return;
            }
            break;

        case MAV_CMD_IMAGE_START_CAPTURE:
            if (!foundCameraAction && missionItem.param1() != 0 && missionItem.param2() == 0 && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                settingsItem->cameraAction()->setRawValue(TakePhotosIntervalTime);
                settingsItem->cameraPhotoIntervalTime()->setRawValue(missionItem.param1());
                visualItems->removeAt(1)->deleteLater();
            } else {
                return;
            }
            break;

        case MAV_CMD_DO_SET_CAM_TRIGG_DIST:
            if (!foundCameraAction && missionItem.param1() != 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                settingsItem->cameraAction()->setRawValue(TakePhotoIntervalDistance);
                settingsItem->cameraPhotoIntervalDistance()->setRawValue(missionItem.param1());
                visualItems->removeAt(1)->deleteLater();
            } else {
                return;
            }
            break;

        case MAV_CMD_VIDEO_START_CAPTURE:
            if (!foundCameraAction && missionItem.param1() == 0 && missionItem.param2() == -1 && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                settingsItem->cameraAction()->setRawValue(TakeVideo);
                visualItems->removeAt(1)->deleteLater();
            } else {
                return;
            }
            break;

        default:
            return;
        }
    }
}

double MissionSettingsComplexItem::complexDistance(void) const
{
    return 0;
}

void MissionSettingsComplexItem::setCruiseSpeed(double cruiseSpeed)
{
    // We don't care about cruise speed
    Q_UNUSED(cruiseSpeed);
}

void MissionSettingsComplexItem::_setDirty(void)
{
    setDirty(true);
}

void MissionSettingsComplexItem::setCoordinate(const QGeoCoordinate& coordinate)
{
    if (this->coordinate() != coordinate) {
        _plannedHomePositionLatitudeFact.setRawValue(coordinate.latitude());
        _plannedHomePositionLongitudeFact.setRawValue(coordinate.longitude());
        _plannedHomePositionAltitudeFact.setRawValue(coordinate.altitude());
    }
}

void MissionSettingsComplexItem::_setDirtyAndUpdateLastSequenceNumber(void)
{
    emit lastSequenceNumberChanged(lastSequenceNumber());
    setDirty(true);
}

void MissionSettingsComplexItem::_setDirtyAndUpdateCoordinate(void)
{
    emit coordinateChanged(coordinate());
    emit exitCoordinateChanged(coordinate());
    setDirty(true);
}

QGeoCoordinate MissionSettingsComplexItem::coordinate(void) const
{
    return QGeoCoordinate(_plannedHomePositionLatitudeFact.rawValue().toDouble(), _plannedHomePositionLongitudeFact.rawValue().toDouble(), _plannedHomePositionAltitudeFact.rawValue().toDouble());
}
