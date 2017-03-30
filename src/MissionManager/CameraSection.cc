/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraSection.h"
#include "SimpleMissionItem.h"

QGC_LOGGING_CATEGORY(CameraSectionLog, "CameraSectionLog")

const char* CameraSection::_gimbalPitchName =                  "GimbalPitch";
const char* CameraSection::_gimbalYawName =                    "GimbalYaw";
const char* CameraSection::_cameraActionName =                 "CameraAction";
const char* CameraSection::_cameraPhotoIntervalDistanceName =  "CameraPhotoIntervalDistance";
const char* CameraSection::_cameraPhotoIntervalTimeName =      "CameraPhotoIntervalTime";

QMap<QString, FactMetaData*> CameraSection::_metaDataMap;

CameraSection::CameraSection(QObject* parent)
    : QObject(parent)
    , _available(false)
    , _settingsSpecified(false)
    , _specifyGimbal(false)
    , _gimbalYawFact                    (0, _gimbalYawName,                     FactMetaData::valueTypeDouble)
    , _gimbalPitchFact                  (0, _gimbalPitchName,                   FactMetaData::valueTypeDouble)
    , _cameraActionFact                 (0, _cameraActionName,                  FactMetaData::valueTypeDouble)
    , _cameraPhotoIntervalDistanceFact  (0, _cameraPhotoIntervalDistanceName,   FactMetaData::valueTypeDouble)
    , _cameraPhotoIntervalTimeFact      (0, _cameraPhotoIntervalTimeName,       FactMetaData::valueTypeUint32)
    , _dirty(false)
{
    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraSection.FactMetaData.json"), NULL /* metaDataParent */);
    }

    _gimbalPitchFact.setMetaData                    (_metaDataMap[_gimbalPitchName]);
    _gimbalYawFact.setMetaData                      (_metaDataMap[_gimbalYawName]);
    _cameraActionFact.setMetaData                   (_metaDataMap[_cameraActionName]);
    _cameraPhotoIntervalDistanceFact.setMetaData    (_metaDataMap[_cameraPhotoIntervalDistanceName]);
    _cameraPhotoIntervalTimeFact.setMetaData        (_metaDataMap[_cameraPhotoIntervalTimeName]);

    _gimbalPitchFact.setRawValue                    (_gimbalPitchFact.rawDefaultValue());
    _gimbalYawFact.setRawValue                      (_gimbalYawFact.rawDefaultValue());
    _cameraActionFact.setRawValue                   (_cameraActionFact.rawDefaultValue());
    _cameraPhotoIntervalDistanceFact.setRawValue    (_cameraPhotoIntervalDistanceFact.rawDefaultValue());
    _cameraPhotoIntervalTimeFact.setRawValue        (_cameraPhotoIntervalTimeFact.rawDefaultValue());

    connect(this,               &CameraSection::specifyGimbalChanged,   this, &CameraSection::_setDirtyAndUpdateMissionItemCount);
    connect(&_cameraActionFact, &Fact::valueChanged,                    this, &CameraSection::_setDirtyAndUpdateMissionItemCount);

    connect(&_gimbalPitchFact,                  &Fact::valueChanged, this, &CameraSection::_setDirty);
    connect(&_gimbalYawFact,                    &Fact::valueChanged, this, &CameraSection::_setDirty);
    connect(&_cameraPhotoIntervalDistanceFact,  &Fact::valueChanged, this, &CameraSection::_setDirty);
    connect(&_cameraPhotoIntervalTimeFact,      &Fact::valueChanged, this, &CameraSection::_setDirty);

    connect(&_gimbalYawFact,                    &Fact::valueChanged, this, &CameraSection::_updateSpecifiedGimbalYaw);
}

void CameraSection::setSpecifyGimbal(bool specifyGimbal)
{
    if (specifyGimbal != _specifyGimbal) {
        _specifyGimbal = specifyGimbal;
        emit specifyGimbalChanged(specifyGimbal);
    }
}

int CameraSection::missionItemCount(void) const
{
    int itemCount = 0;

    if (_specifyGimbal) {
        itemCount++;
    }
    if (_cameraActionFact.rawValue().toInt() != CameraActionNone) {
        itemCount++;
    }

    return itemCount;
}

void CameraSection::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        emit dirtyChanged(_dirty);
    }
}

void CameraSection::appendMissionItems(QList<MissionItem*>& items, QObject* missionItemParent, int nextSequenceNumber)
{
    // IMPORTANT NOTE: If anything changes here you must also change CameraSection::scanForMissionSettings

    if (_specifyGimbal) {
        MissionItem* item = new MissionItem(nextSequenceNumber++,
                                            MAV_CMD_DO_MOUNT_CONTROL,
                                            MAV_FRAME_MISSION,
                                            _gimbalPitchFact.rawValue().toDouble(),
                                            0,                                      // Gimbal roll
                                            _gimbalYawFact.rawValue().toDouble(),
                                            0, 0, 0,                                // param 4-6 not used
                                            MAV_MOUNT_MODE_MAVLINK_TARGETING,
                                            true,                                   // autoContinue
                                            false,                                  // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }

    if (_cameraActionFact.rawValue().toInt() != CameraActionNone) {
        MissionItem* item = NULL;

        switch (_cameraActionFact.rawValue().toInt()) {
        case TakePhotosIntervalTime:
            item = new MissionItem(nextSequenceNumber++,
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
                                   missionItemParent);
            break;

        case TakePhotoIntervalDistance:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                   MAV_FRAME_MISSION,
                                   _cameraPhotoIntervalDistanceFact.rawValue().toDouble(),  // Trigger distance
                                   0, 0, 0, 0, 0, 0,                                        // param 2-7 not used
                                   true,                                                    // autoContinue
                                   false,                                                   // isCurrentItem
                                   missionItemParent);
            break;

        case TakeVideo:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_VIDEO_START_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                           // Camera ID
                                   -1,                          // Max fps
                                   -1,                          // Max resolution
                                   0, 0, 0, 0,                  // param 5-7 not used
                                   true,                        // autoContinue
                                   false,                       // isCurrentItem
                                   missionItemParent);
            break;

        case StopTakingVideo:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_VIDEO_STOP_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                           // Camera ID
                                   0, 0, 0, 0, 0, 0,            // param 2-7 not used
                                   true,                        // autoContinue
                                   false,                       // isCurrentItem
                                   missionItemParent);
            break;

        case StopTakingPhotos:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                   MAV_FRAME_MISSION,
                                   0,                               // Trigger distance = 0 means stop
                                   0, 0, 0, 0, 0, 0,                // param 2-7 not used
                                   true,                            // autoContinue
                                   false,                           // isCurrentItem
                                   missionItemParent);
            items.append(item);
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_IMAGE_STOP_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                           // camera id
                                   0, 0, 0, 0, 0, 0,            // param 2-7 not used
                                   true,                        // autoContinue
                                   false,                       // isCurrentItem
                                   missionItemParent);
            break;
        }
        if (item) {
            items.append(item);
        }
    }
}

bool CameraSection::scanForCameraSection(QmlObjectListModel* visualItems, int scanIndex)
{
    bool foundGimbal = false;
    bool foundCameraAction = false;
    bool stopLooking = false;

    qCDebug(CameraSectionLog) << "CameraSection::scanForCameraSection" << visualItems->count() << scanIndex;

    // Scan through the initial mission items for possible mission settings

    while (!stopLooking && visualItems->count() > scanIndex) {
        SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
        if (!item) {
            // We hit a complex item there can be no more possible mission settings
            return foundGimbal || foundCameraAction;
        }
        MissionItem& missionItem = item->missionItem();

        qCDebug(CameraSectionLog) << item->command() << missionItem.param1() << missionItem.param2() << missionItem.param3() << missionItem.param4() << missionItem.param5() << missionItem.param6() << missionItem.param7() ;

        // See CameraSection::appendMissionItems for specs on what compomises a known camera section item

        switch ((MAV_CMD)item->command()) {
        case MAV_CMD_DO_MOUNT_CONTROL:
            if (!foundGimbal && missionItem.param2() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == MAV_MOUNT_MODE_MAVLINK_TARGETING) {
                foundGimbal = true;
                setSpecifyGimbal(true);
                gimbalPitch()->setRawValue(missionItem.param1());
                gimbalYaw()->setRawValue(missionItem.param3());
                visualItems->removeAt(scanIndex)->deleteLater();
            } else {
                stopLooking = true;
            }
            break;

        case MAV_CMD_IMAGE_START_CAPTURE:
            if (!foundCameraAction && missionItem.param1() != 0 && missionItem.param2() == 0 && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                cameraAction()->setRawValue(TakePhotosIntervalTime);
                cameraPhotoIntervalTime()->setRawValue(missionItem.param1());
                visualItems->removeAt(scanIndex)->deleteLater();
            }
            stopLooking = true;
            break;

        case MAV_CMD_DO_SET_CAM_TRIGG_DIST:
            if (!foundCameraAction && missionItem.param1() >= 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                // At this point we don't know if we have a stop taking photos pair, or just a distance trigger

                if (missionItem.param1() == 0 && scanIndex < visualItems->count() - 1) {
                    // Possible stop taking photos pair
                    SimpleMissionItem* nextItem = visualItems->value<SimpleMissionItem*>(scanIndex + 1);
                    if (nextItem) {
                        MissionItem& nextMissionItem = nextItem->missionItem();
                        if (nextMissionItem.command() == MAV_CMD_IMAGE_STOP_CAPTURE && nextMissionItem.param1() == 0 && nextMissionItem.param2() == 0 && nextMissionItem.param3() == 0 && nextMissionItem.param4() == 0 && nextMissionItem.param5() == 0 && nextMissionItem.param6() == 0 && nextMissionItem.param7() == 0) {
                            // We found a stop taking photos pair
                            foundCameraAction = true;
                            cameraAction()->setRawValue(StopTakingPhotos);
                            visualItems->removeAt(scanIndex)->deleteLater();
                            visualItems->removeAt(scanIndex)->deleteLater();
                            stopLooking = true;
                            break;
                        }
                    }
                }

                // We didn't find a stop taking photos pair, check for trigger distance
                if (missionItem.param1() > 0) {
                    foundCameraAction = true;
                    cameraAction()->setRawValue(TakePhotoIntervalDistance);
                    cameraPhotoIntervalDistance()->setRawValue(missionItem.param1());
                    visualItems->removeAt(scanIndex)->deleteLater();
                    stopLooking = true;
                    break;
                }
            }
            stopLooking = true;
            break;

        case MAV_CMD_VIDEO_START_CAPTURE:
            if (!foundCameraAction && missionItem.param1() == 0 && missionItem.param2() == -1 && missionItem.param3() == -1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                cameraAction()->setRawValue(TakeVideo);
                visualItems->removeAt(scanIndex)->deleteLater();
            }
            stopLooking = true;
            break;

        case MAV_CMD_VIDEO_STOP_CAPTURE:
            if (!foundCameraAction && missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                cameraAction()->setRawValue(StopTakingVideo);
                visualItems->removeAt(scanIndex)->deleteLater();
            }
            stopLooking = true;
            break;

        default:
            stopLooking = true;
            break;
        }
    }

    qCDebug(CameraSectionLog) << foundGimbal << foundCameraAction;

    _settingsSpecified = foundGimbal || foundCameraAction;
    emit settingsSpecifiedChanged(_settingsSpecified);

    return foundGimbal || foundCameraAction;
}

void CameraSection::_setDirty(void)
{
    setDirty(true);
}

void CameraSection::_setDirtyAndUpdateMissionItemCount(void)
{
    emit missionItemCountChanged(missionItemCount());
    setDirty(true);
}

void CameraSection::setAvailable(bool available)
{
    if (_available != available) {
        _available = available;
        emit availableChanged(available);
    }
}

double CameraSection::specifiedGimbalYaw(void) const
{
    return _specifyGimbal ? _gimbalYawFact.rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void CameraSection::_updateSpecifiedGimbalYaw(void)
{
    emit specifiedGimbalYawChanged(specifiedGimbalYaw());
}
