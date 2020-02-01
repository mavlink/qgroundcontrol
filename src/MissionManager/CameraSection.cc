/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/CameraSection.FactMetaData.json"), Q_NULLPTR /* metaDataParent */);
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

    connect(&_gimbalPitchFact,                  &Fact::valueChanged,                        this, &CameraSection::_dirtyIfSpecified);
    connect(&_gimbalYawFact,                    &Fact::valueChanged,                        this, &CameraSection::_dirtyIfSpecified);
    connect(&_cameraPhotoIntervalDistanceFact,  &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(&_cameraPhotoIntervalTimeFact,      &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(&_cameraModeFact,                   &Fact::valueChanged,                        this, &CameraSection::_setDirty);
    connect(this,                               &CameraSection::specifyGimbalChanged,       this, &CameraSection::_setDirty);
    connect(this,                               &CameraSection::specifyCameraModeChanged,   this, &CameraSection::_setDirty);

    connect(&_gimbalYawFact,                    &Fact::valueChanged,                        this, &CameraSection::_updateSpecifiedGimbalYaw);
    connect(&_gimbalPitchFact,                  &Fact::valueChanged,                        this, &CameraSection::_updateSpecifiedGimbalPitch);
}

void CameraSection::setSpecifyGimbal(bool specifyGimbal)
{
    if (specifyGimbal != _specifyGimbal) {
        _specifyGimbal = specifyGimbal;
        emit specifyGimbalChanged(specifyGimbal);
        emit specifiedGimbalYawChanged(specifiedGimbalYaw());
        emit specifiedGimbalPitchChanged(specifiedGimbalPitch());
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
                                            0,                                              // Reserved (Set to 0)
                                            _cameraModeFact.rawValue().toDouble(),
                                            qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(),    // reserved
                                            true,                                           // autoContinue
                                            false,                                          // isCurrentItem
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
        MissionItem* item = nullptr;

        switch (_cameraActionFact.rawValue().toInt()) {
        case TakePhotosIntervalTime:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_IMAGE_START_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                                               // Reserved (Set to 0)
                                   _cameraPhotoIntervalTimeFact.rawValue().toInt(), // Interval
                                   0,                                               // Unlimited photo count
                                   qQNaN(), qQNaN(), qQNaN(), qQNaN(),              // reserved
                                   true,                                            // autoContinue
                                   false,                                           // isCurrentItem
                                   missionItemParent);
            break;

        case TakePhotoIntervalDistance:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                   MAV_FRAME_MISSION,
                                   _cameraPhotoIntervalDistanceFact.rawValue().toDouble(),  // Trigger distance
                                   0,                                                       // No shutter integartion
                                   1,                                                       // Trigger immediately
                                   0, 0, 0, 0,                                              // param 4-7 not used
                                   true,                                                    // autoContinue
                                   false,                                                   // isCurrentItem
                                   missionItemParent);
            break;

        case TakeVideo:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_VIDEO_START_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                                               // Reserved (Set to 0)
                                   VIDEO_CAPTURE_STATUS_INTERVAL,                   // CAMERA_CAPTURE_STATUS (default to every 5 seconds)
                                   qQNaN(), qQNaN(), qQNaN(), qQNaN(),  qQNaN(),    // reserved
                                   true,                                            // autoContinue
                                   false,                                           // isCurrentItem
                                   missionItemParent);
            break;

        case StopTakingVideo:
            appendStopTakingVideo(items, nextSequenceNumber, missionItemParent);
            break;

        case StopTakingPhotos:
            appendStopTakingPhotos(items, nextSequenceNumber, missionItemParent);
            break;

        case TakePhoto:
            item = new MissionItem(nextSequenceNumber++,
                                   MAV_CMD_IMAGE_START_CAPTURE,
                                   MAV_FRAME_MISSION,
                                   0,                           // Reserved (Set to 0)
                                   0,                           // Interval (none)
                                   1,                           // Take 1 photo
                                   0,                           // No sequence number specified
                                   qQNaN(), qQNaN(), qQNaN(),   // reserved
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

void CameraSection::appendStopTakingPhotos(QList<MissionItem*>& items, int& seqNum, QObject* missionItemParent)
{
    MissionItem* item = new MissionItem(seqNum++,
                           MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                           MAV_FRAME_MISSION,
                           0,                               // Trigger distance = 0 means stop
                           0, 0, 0, 0, 0, 0,                // param 2-7 not used
                           true,                            // autoContinue
                           false,                           // isCurrentItem
                           missionItemParent);
    items.append(item);
    item = new MissionItem(seqNum++,
                           MAV_CMD_IMAGE_STOP_CAPTURE,
                           MAV_FRAME_MISSION,
                           0,                                                       // Reserved (Set to 0)
                           qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(),    // reserved
                           true,                                                    // autoContinue
                           false,                                                   // isCurrentItem
                           missionItemParent);
    items.append(item);
}

void CameraSection::appendStopTakingVideo(QList<MissionItem*>& items, int& seqNum, QObject* missionItemParent)
{
    MissionItem* item = new MissionItem(seqNum++,
                           MAV_CMD_VIDEO_STOP_CAPTURE,
                           MAV_FRAME_MISSION,
                           0,                                                       // Reserved (Set to 0)
                           qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(),    // reserved
                           true,                                                    // autoContinue
                           false,                                                   // isCurrentItem
                           missionItemParent);
    items.append(item);
}

bool CameraSection::_scanGimbal(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_DO_MOUNT_CONTROL) {
            if (missionItem.param2() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == MAV_MOUNT_MODE_MAVLINK_TARGETING) {
                setSpecifyGimbal(true);
                gimbalPitch()->setRawValue(missionItem.param1());
                gimbalYaw()->setRawValue(missionItem.param3());
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::_scanTakePhoto(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_IMAGE_START_CAPTURE) {
            if (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 1) {
                cameraAction()->setRawValue(TakePhoto);
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::_scanTakePhotosIntervalTime(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_IMAGE_START_CAPTURE) {
            if (missionItem.param1() == 0 && missionItem.param2() >= 1 && missionItem.param3() == 0) {
                cameraAction()->setRawValue(TakePhotosIntervalTime);
                cameraPhotoIntervalTime()->setRawValue(missionItem.param2());
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::scanStopTakingPhotos(QmlObjectListModel* visualItems, int scanIndex, bool removeScannedItems)
{
    if (scanIndex < 0 || scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
            if (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                if (scanIndex < visualItems->count() - 1) {
                    SimpleMissionItem* nextItem = visualItems->value<SimpleMissionItem*>(scanIndex + 1);
                    if (nextItem) {
                        MissionItem& nextMissionItem = nextItem->missionItem();
                        if (nextMissionItem.command() == MAV_CMD_IMAGE_STOP_CAPTURE && nextMissionItem.param1() == 0) {
                            if (removeScannedItems) {
                                visualItems->removeAt(scanIndex)->deleteLater();
                                visualItems->removeAt(scanIndex)->deleteLater();
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool CameraSection::_scanTriggerStartDistance(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex < 0 || scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
            if (missionItem.param1() > 0 && missionItem.param2() == 0 && missionItem.param3() == 1 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                cameraAction()->setRawValue(TakePhotoIntervalDistance);
                cameraPhotoIntervalDistance()->setRawValue(missionItem.param1());
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::_scanTriggerStopDistance(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex < 0 || scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_DO_SET_CAM_TRIGG_DIST) {
            if (missionItem.param1() == 0 && missionItem.param2() == 0 && missionItem.param3() == 0 && missionItem.param4() == 0 && missionItem.param5() == 0 && missionItem.param6() == 0 && missionItem.param7() == 0) {
                cameraAction()->setRawValue(TakePhotoIntervalDistance);
                cameraPhotoIntervalDistance()->setRawValue(missionItem.param1());
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::_scanTakeVideo(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_VIDEO_START_CAPTURE) {
            if (missionItem.param1() == 0 && missionItem.param2() == VIDEO_CAPTURE_STATUS_INTERVAL) {
                cameraAction()->setRawValue(TakeVideo);
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::scanStopTakingVideo(QmlObjectListModel* visualItems, int scanIndex, bool removeScannedItems)
{
    if (scanIndex < 0 || scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_VIDEO_STOP_CAPTURE) {
            if (missionItem.param1() == 0) {
                if (removeScannedItems) {
                    visualItems->removeAt(scanIndex)->deleteLater();
                }
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::_scanSetCameraMode(QmlObjectListModel* visualItems, int scanIndex)
{
    if (scanIndex < 0 || scanIndex > visualItems->count() -1) {
        return false;
    }
    SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(scanIndex);
    if (item) {
        MissionItem& missionItem = item->missionItem();
        if ((MAV_CMD)item->command() == MAV_CMD_SET_CAMERA_MODE) {
            // We specifically don't test param 5/6/7 since we don't have NaN persistence for those fields
            if (missionItem.param1() == 0 && (missionItem.param2() == CAMERA_MODE_IMAGE || missionItem.param2() == CAMERA_MODE_VIDEO || missionItem.param2() == CAMERA_MODE_IMAGE_SURVEY) && qIsNaN(missionItem.param3())) {
                setSpecifyCameraMode(true);
                cameraMode()->setRawValue(missionItem.param2());
                visualItems->removeAt(scanIndex)->deleteLater();
                return true;
            }
        }
    }

    return false;
}

bool CameraSection::scanForSection(QmlObjectListModel* visualItems, int scanIndex)
{
    bool foundGimbal = false;
    bool foundCameraAction = false;
    bool foundCameraMode = false;

    qCDebug(CameraSectionLog) << "CameraSection::scanForCameraSection visualItems->count():scanIndex;" << visualItems->count() << scanIndex;

    if (!_available || scanIndex >= visualItems->count()) {
        return false;
    }

    // Scan through the initial mission items for possible mission settings

    while (visualItems->count() > scanIndex) {
        if (!foundGimbal && _scanGimbal(visualItems, scanIndex)) {
            foundGimbal = true;
            continue;
        }
        if (!foundCameraAction && _scanTakePhoto(visualItems, scanIndex)) {
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraAction && _scanTakePhotosIntervalTime(visualItems, scanIndex)) {
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraAction && scanStopTakingPhotos(visualItems, scanIndex, true /* removeScannedItems */)) {
            cameraAction()->setRawValue(StopTakingPhotos);
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraAction && _scanTriggerStartDistance(visualItems, scanIndex)) {
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraAction && _scanTriggerStopDistance(visualItems, scanIndex)) {
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraAction && _scanTakeVideo(visualItems, scanIndex)) {
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraAction && scanStopTakingVideo(visualItems, scanIndex, true /* removeScannedItems */)) {
            cameraAction()->setRawValue(StopTakingVideo);
            foundCameraAction = true;
            continue;
        }
        if (!foundCameraMode && _scanSetCameraMode(visualItems, scanIndex)) {
            foundCameraMode = true;
            continue;
        }
        break;
    }

    qCDebug(CameraSectionLog) << "CameraSection::scanForCameraSection foundGimbal:foundCameraAction:foundCameraMode;" << foundGimbal << foundCameraAction << foundCameraMode;

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

double CameraSection::specifiedGimbalPitch(void) const
{
    return _specifyGimbal ? _gimbalPitchFact.rawValue().toDouble() : std::numeric_limits<double>::quiet_NaN();
}

void CameraSection::_updateSpecifiedGimbalYaw(void)
{
    if (_specifyGimbal) {
        emit specifiedGimbalYawChanged(specifiedGimbalYaw());
    }
}

void CameraSection::_updateSpecifiedGimbalPitch(void)
{
    if (_specifyGimbal) {
        emit specifiedGimbalPitchChanged(specifiedGimbalPitch());
    }
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
    return _specifyCameraMode || _vehicle->firmwarePlugin()->supportedMissionCommands().contains(MAV_CMD_SET_CAMERA_MODE);
}

void CameraSection::_dirtyIfSpecified(void)
{
    // We only set the dirty bit if specify gimbal it set. This allows us to change defaults without affecting dirty.
    if (_specifyGimbal) {
        setDirty(true);
    }
}
