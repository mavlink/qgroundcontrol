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
#include "FirmwarePlugin.h"

QGC_LOGGING_CATEGORY(CameraSectionLog, "CameraSectionLog")

const char* CameraSection::_gimbalPitchName =                   "GimbalPitch";
const char* CameraSection::_gimbalYawName =                     "GimbalYaw";
const char* CameraSection::_cameraActionName =                  "CameraAction";
const char* CameraSection::_cameraPhotoIntervalDistanceName =   "CameraPhotoIntervalDistance";
const char* CameraSection::_cameraPhotoIntervalTimeName =       "CameraPhotoIntervalTime";
const char* CameraSection::_cameraModeName =                    "CameraMode";

QMap<QString, FactMetaData*> CameraSection::_metaDataMap;

CameraSection::CameraSection(Vehicle* vehicle, QObject* parent)
    : Section(vehicle, parent)
    , _available(false)
    , _settingsSpecified(false)
    , _specifyGimbal(false)
    , _specifyCameraMode(false)
    , _gimbalYawFact                    (0, _gimbalYawName,                     FactMetaData::valueTypeDouble)
    , _gimbalPitchFact                  (0, _gimbalPitchName,                   FactMetaData::valueTypeDouble)
    , _cameraActionFact                 (0, _cameraActionName,                  FactMetaData::valueTypeDouble)
    , _cameraPhotoIntervalDistanceFact  (0, _cameraPhotoIntervalDistanceName,   FactMetaData::valueTypeDouble)
    , _cameraPhotoIntervalTimeFact      (0, _cameraPhotoIntervalTimeName,       FactMetaData::valueTypeUint32)
    , _cameraModeFact                   (0, _cameraModeName,                    FactMetaData::valueTypeUint32)
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
    _cameraModeFact.setMetaData                     (_metaDataMap[_cameraModeName]);

    _gimbalPitchFact.setRawValue                    (_gimbalPitchFact.rawDefaultValue());
    _gimbalYawFact.setRawValue                      (_gimbalYawFact.rawDefaultValue());
    _cameraActionFact.setRawValue                   (_cameraActionFact.rawDefaultValue());
    _cameraPhotoIntervalDistanceFact.setRawValue    (_cameraPhotoIntervalDistanceFact.rawDefaultValue());
    _cameraPhotoIntervalTimeFact.setRawValue        (_cameraPhotoIntervalTimeFact.rawDefaultValue());
    _cameraModeFact.setRawValue                     (_cameraModeFact.rawDefaultValue());

    connect(this,                               &CameraSection::specifyGimbalChanged,       this, &CameraSection::_specifyChanged);
    connect(this,                               &CameraSection::specifyCameraModeChanged,   this, &CameraSection::_specifyChanged);

    connect(&_cameraActionFact,                 &Fact::valueChanged,                        this, &CameraSection::_cameraActionChanged);

    connect(&_gimbalPitchFact,                  &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(&_gimbalYawFact,                    &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(&_cameraPhotoIntervalDistanceFact,  &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(&_cameraPhotoIntervalTimeFact,      &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(&_cameraModeFact,                   &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(this,                               &CameraSection::specifyGimbalChanged,       this, &CameraSection::_setDirty);
    connect(this,                               &CameraSection::specifyCameraModeChanged,   this, &CameraSection::_setDirty);

    connect(&_gimbalYawFact,                    &Fact::valueChanged,                        this, &CameraSection::_updateSpecifiedGimbalYaw);
}

void CameraSection::setSpecifyGimbal(bool specifyGimbal)
{
    if (specifyGimbal != _specifyGimbal) {
        _specifyGimbal = specifyGimbal;
        emit specifyGimbalChanged(specifyGimbal);
    }
}

void CameraSection::setSpecifyCameraMode(bool specifyCameraMode)
{
    if (specifyCameraMode != _specifyCameraMode) {
        _specifyCameraMode = specifyCameraMode;
        emit specifyCameraModeChanged(specifyCameraMode);
    }
}

int CameraSection::itemCount(void) const
{
    int itemCount = 0;

    if (_specifyGimbal) {
        itemCount++;
    }
    if (_specifyCameraMode) {
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

void CameraSection::appendSectionItems(QList<MissionItem*>& items, QObject* missionItemParent, int& nextSequenceNumber)
{
    // IMPORTANT NOTE: If anything changes here you must also change CameraSection::scanForSection

    if (_specifyCameraMode) {
        MissionItem* item = new MissionItem(nextSequenceNumber++,
                                            MAV_CMD_SET_CAMERA_MODE,
                                            MAV_FRAME_MISSION,
                                            0,                                      // camera id, all cameras
                                            _cameraModeFact.rawValue().toDouble(),
                                            NAN, NAN, NAN, NAN, NAN,                // param 3-7 unused
                                            true,                                   // autoContinue
                                            false,                                  // isCurrentItem
                                            missionItemParent);
        items.append(item);
    }

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
                                   0,                                               // Camera ID, all cameras
                                   _cameraPhotoIntervalTimeFact.rawValue().toInt(), // Interval
                                   0,                                               // Unlimited photo count
                                   -1,                                              // Max horizontal resolution
                                   -1,                                              // Max vertical resolution
                                   0, 0,                                            // param 6-7 not used
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
                                   0,                           // Camera ID, all cameras
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
                                   0,                           // Camera ID, all cameras
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
                                   0,                           // camera id, all cameras
                                   0, 0, 0, 0, 0, 0,            // param 2-7 not used
                                   true,                        // autoContinue
                                   false,                       // isCurrentItem
                                   missionItemParent);
            break;

        case TakePhoto:
                item = new MissionItem(nextSequenceNumber++,
                                       MAV_CMD_IMAGE_START_CAPTURE,
                                       MAV_FRAME_MISSION,
                                       0,                               // camera id = 0, all cameras
                                       0,                              // Interval (none)
                                       1,                              // Take 1 photo
                                       -1,                             // Max horizontal resolution
                                       -1,                             // Max vertical resolution
                                       0, 0,                           // param 6-7 not used
                                       true,                           // autoContinue
                                       false,                         // isCurrentItem
                                       missionItemParent);
            break;
        }
        if (item) {
            items.append(item);
        }
    }
}

bool CameraSection::scanForSection(QmlObjectListModel* visualItems, int scanIndex)
{
    bool foundGimbal = false;
    bool foundCameraAction = false;
    bool foundCameraMode = false;
    bool stopLooking = false;

    qCDebug(CameraSectionLog) << "CameraSection::scanForCameraSection" << visualItems->count() << scanIndex;

    if (!_available || scanIndex >= visualItems->count()) {
        return false;
    }

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
            // This could possibly be TakePhotosIntervalTime or TakePhoto
            if (!foundCameraAction &&
                    // TakePhotosIntervalTime matching
                    ((missionItem.param1() == 0 && missionItem.param2() >= 1 && missionItem.param3() == 0 && missionItem.param4() == -1 && missionItem.param5() == -1 && missionItem.param6() == 0 && missionItem.param7() == 0) ||
                    // TakePhoto matching
                    (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 1 && missionItem.param4() == -1 && missionItem.param5() == -1 && missionItem.param6() == 0 && missionItem.param7() == 0))) {
                foundCameraAction = true;
                if (missionItem.param2() == 0) {
                    cameraAction()->setRawValue(TakePhoto);
                } else {
                    cameraAction()->setRawValue(TakePhotosIntervalTime);
                    cameraPhotoIntervalTime()->setRawValue(missionItem.param2());
                }
                visualItems->removeAt(scanIndex)->deleteLater();
            } else {
                stopLooking = true;
            }
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
            } else {
                stopLooking = true;
            }
            break;

        case MAV_CMD_VIDEO_STOP_CAPTURE:
            if (!foundCameraAction && missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                foundCameraAction = true;
                cameraAction()->setRawValue(StopTakingVideo);
                visualItems->removeAt(scanIndex)->deleteLater();
            } else {
                stopLooking = true;
            }
            break;

        case MAV_CMD_SET_CAMERA_MODE:
            // We specifically don't test param 5/6/7 since we don't have NaN persistence for those fields
            if (!foundCameraMode && missionItem.param1() == 0 && (missionItem.param2() == 0 || missionItem.param2() == 1) && qIsNaN(missionItem.param3())) {
                foundCameraMode = true;
                setSpecifyCameraMode(true);
                cameraMode()->setRawValue(missionItem.param2());
                visualItems->removeAt(scanIndex)->deleteLater();
            } else {
                stopLooking = true;
            }
            break;

        default:
            stopLooking = true;
            break;
        }
    }

    qCDebug(CameraSectionLog) << foundGimbal << foundCameraAction << foundCameraMode;

    _settingsSpecified = foundGimbal || foundCameraAction || foundCameraMode;
    emit settingsSpecifiedChanged(_settingsSpecified);

    return _settingsSpecified;
}

void CameraSection::_setDirty(void)
{
    setDirty(true);
}

void CameraSection::_setDirtyAndUpdateItemCount(void)
{
    emit itemCountChanged(itemCount());
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

void CameraSection::_updateSettingsSpecified(void)
{
    bool newSettingsSpecified = _specifyGimbal || _specifyCameraMode || _cameraActionFact.rawValue().toInt() != CameraActionNone;
    if (newSettingsSpecified != _settingsSpecified) {
        _settingsSpecified = newSettingsSpecified;
        emit settingsSpecifiedChanged(newSettingsSpecified);
    }
}

void CameraSection::_specifyChanged(void)
{
    _setDirtyAndUpdateItemCount();
    _updateSettingsSpecified();
}

void CameraSection::_cameraActionChanged(void)
{
    _setDirtyAndUpdateItemCount();
    _updateSettingsSpecified();
}

bool CameraSection::cameraModeSupported(void) const
{
    return _vehicle->firmwarePlugin()->supportedMissionCommands().contains(MAV_CMD_SET_CAMERA_MODE);
}
