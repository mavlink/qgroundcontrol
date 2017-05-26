/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraSectionTest.h"

CameraSectionTest::CameraSectionTest(void)
    : _spyCamera(NULL)
    , _spySection(NULL)
    , _cameraSection(NULL)
    , _validGimbalItem(NULL)
    , _validDistanceItem(NULL)
    , _validTimeItem(NULL)
    , _validStartVideoItem(NULL)
    , _validStopVideoItem(NULL)
    , _validStopDistanceItem(NULL)
    , _validStopTimeItem(NULL)
{
    
}

void CameraSectionTest::init(void)
{
    SectionTest::init();

    rgCameraSignals[specifyGimbalChangedIndex] =        SIGNAL(specifyGimbalChanged(bool));
    rgCameraSignals[specifiedGimbalYawChangedIndex] =   SIGNAL(specifiedGimbalYawChanged(double));
    rgCameraSignals[specifyCameraModeChangedIndex] =    SIGNAL(specifyCameraModeChanged(bool));

    _cameraSection = _simpleItem->cameraSection();
    _createSpy(_cameraSection, &_spyCamera);
    QVERIFY(_spyCamera);
    SectionTest::_createSpy(_cameraSection, &_spySection);
    QVERIFY(_spySection);

    _validGimbalItem = new SimpleMissionItem(_offlineVehicle,
                                             MissionItem(0, MAV_CMD_DO_MOUNT_CONTROL, MAV_FRAME_MISSION, 10.1234, 0, 20.1234, 0, 0, 0, MAV_MOUNT_MODE_MAVLINK_TARGETING, true, false),
                                             this);
    _validTimeItem = new SimpleMissionItem(_offlineVehicle,
                                           MissionItem(0, MAV_CMD_IMAGE_START_CAPTURE, MAV_FRAME_MISSION, 0, 48, 0, -1, -1, 0, 0, true, false),
                                           this);
    _validDistanceItem = new SimpleMissionItem(_offlineVehicle,
                                               MissionItem(0, MAV_CMD_DO_SET_CAM_TRIGG_DIST, MAV_FRAME_MISSION, 72, 0, 0, 0, 0, 0, 0, true, false),
                                               this);
    _validStartVideoItem = new SimpleMissionItem(_offlineVehicle,
                                                 MissionItem(0, MAV_CMD_VIDEO_START_CAPTURE, MAV_FRAME_MISSION, 0, -1, -1, 0, 0, 0, 0, true, false),
                                                 this);
    _validStopVideoItem = new SimpleMissionItem(_offlineVehicle,
                                                MissionItem(0, MAV_CMD_VIDEO_STOP_CAPTURE, MAV_FRAME_MISSION, 0, 0, 0, 0, 0, 0, 0, true, false),
                                                this);
    _validStopDistanceItem = new SimpleMissionItem(_offlineVehicle,
                                                   MissionItem(0, MAV_CMD_DO_SET_CAM_TRIGG_DIST, MAV_FRAME_MISSION, 0, 0, 0, 0, 0, 0, 0, true, false),
                                                   this);
    _validStopTimeItem = new SimpleMissionItem(_offlineVehicle,
                                               MissionItem(1, MAV_CMD_IMAGE_STOP_CAPTURE, MAV_FRAME_MISSION, 0, 0, 0, 0, 0, 0, 0, true, false),
                                               this);
    _validCameraPhotoModeItem = new SimpleMissionItem(_offlineVehicle,
                                               MissionItem(0,                       // sequence number
                                                           MAV_CMD_SET_CAMERA_MODE,
                                                           MAV_FRAME_MISSION,
                                                           0,                       // camera id = 0, all cameras
                                                           0,                       // photo mode
                                                           NAN, NAN, NAN, NAN, NAN, // param 3-7 unused
                                                           true,                    // autocontinue
                                                           false),                  // isCurrentItem
                                               this);
    _validCameraVideoModeItem = new SimpleMissionItem(_offlineVehicle,
                                               MissionItem(0,                       // sequence number
                                                           MAV_CMD_SET_CAMERA_MODE,
                                                           MAV_FRAME_MISSION,
                                                           0,                       // camera id = 0, all cameras
                                                           1,                       // video mode
                                                           NAN, NAN, NAN, NAN, NAN, // param 3-7 unused
                                                           true,                    // autocontinue
                                                           false),                  // isCurrentItem
                                               this);
}

void CameraSectionTest::cleanup(void)
{
    delete _spyCamera;
    delete _spySection;
    delete _validGimbalItem;
    delete _validDistanceItem;
    delete _validTimeItem;
    delete _validStartVideoItem;
    delete _validStopVideoItem;
    delete _validStopDistanceItem;
    delete _validStopTimeItem;
    SectionTest::cleanup();
}

void CameraSectionTest::_createSpy(CameraSection* cameraSection, MultiSignalSpy** cameraSpy)
{
    *cameraSpy = NULL;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QCOMPARE(spy->init(cameraSection, rgCameraSignals, cCameraSignals), true);
    *cameraSpy = spy;
}

void CameraSectionTest::_testDirty(void)
{
    // Check for dirty not signalled if same value

    _cameraSection->setSpecifyGimbal(_cameraSection->specifyGimbal());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    _cameraSection->setSpecifyCameraMode(_cameraSection->specifyCameraMode());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    _cameraSection->gimbalPitch()->setRawValue(_cameraSection->gimbalPitch()->rawValue());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    _cameraSection->gimbalYaw()->setRawValue(_cameraSection->gimbalPitch()->rawValue());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    _cameraSection->cameraAction()->setRawValue(_cameraSection->cameraAction()->rawValue());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    _cameraSection->cameraPhotoIntervalTime()->setRawValue(_cameraSection->cameraPhotoIntervalTime()->rawValue());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    _cameraSection->cameraPhotoIntervalDistance()->setRawValue(_cameraSection->cameraPhotoIntervalDistance()->rawValue());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_cameraSection->dirty(), false);

    // Check for no duplicate dirty signalling on change

    _cameraSection->setSpecifyGimbal(!_cameraSection->specifyGimbal());
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _spySection->clearAllSignals();

    _cameraSection->setSpecifyGimbal(!_cameraSection->specifyGimbal());
    QVERIFY(_spySection->checkNoSignalByMask(dirtyChangedMask));
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    _cameraSection->setSpecifyCameraMode(!_cameraSection->specifyCameraMode());
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _spySection->clearAllSignals();

    _cameraSection->setSpecifyCameraMode(!_cameraSection->specifyCameraMode());
    QVERIFY(_spySection->checkNoSignalByMask(dirtyChangedMask));
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    // Check the remaining items that should set dirty bit

    _cameraSection->gimbalPitch()->setRawValue(_cameraSection->gimbalPitch()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    _cameraSection->gimbalYaw()->setRawValue(_cameraSection->gimbalPitch()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    _cameraSection->cameraAction()->setRawValue(_cameraSection->cameraAction()->rawValue().toInt() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    _cameraSection->cameraPhotoIntervalTime()->setRawValue(_cameraSection->cameraPhotoIntervalTime()->rawValue().toInt() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    _cameraSection->cameraPhotoIntervalDistance()->setRawValue(_cameraSection->cameraPhotoIntervalDistance()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    _cameraSection->cameraMode()->setRawValue(_cameraSection->cameraMode()->rawValue().toInt() == 0 ? 1 : 0);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();
}

void CameraSectionTest::_testSettingsAvailable(void)
{
    // No settings specified to start
    QVERIFY(_cameraSection->cameraAction()->rawValue().toInt() == CameraSection::CameraActionNone);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);

    // Check correct reaction to specify methods on/off

    _cameraSection->setSpecifyGimbal(true);
    QCOMPARE(_cameraSection->specifyGimbal(), true);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QVERIFY(_spyCamera->checkSignalByMask(specifyGimbalChangedMask));
    QCOMPARE(_spyCamera->pullBoolFromSignalIndex(specifyGimbalChangedIndex), true);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), true);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyGimbal(false);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    QVERIFY(_spyCamera->checkSignalByMask(specifyGimbalChangedMask));
    QCOMPARE(_spyCamera->pullBoolFromSignalIndex(specifyGimbalChangedIndex), false);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), false);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyCameraMode(true);
    QCOMPARE(_cameraSection->specifyCameraMode(), true);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QVERIFY(_spyCamera->checkSignalByMask(specifyCameraModeChangedMask));
    QCOMPARE(_spyCamera->pullBoolFromSignalIndex(specifyCameraModeChangedIndex), true);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), true);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyCameraMode(false);
    QCOMPARE(_cameraSection->specifyCameraMode(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    QVERIFY(_spyCamera->checkSignalByMask(specifyCameraModeChangedMask));
    QCOMPARE(_spyCamera->pullBoolFromSignalIndex(specifyCameraModeChangedIndex), false);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), false);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    // Check correct reaction to cameraAction on/off

    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    QVERIFY(_cameraSection->cameraAction()->rawValue().toInt() == CameraSection::TakePhotosIntervalTime);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), true);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    QVERIFY(_cameraSection->cameraAction()->rawValue().toInt() == CameraSection::CameraActionNone);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), false);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    // Check that there is not multiple signalling of settingsSpecified

    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    _cameraSection->setSpecifyGimbal(true);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
}

void CameraSectionTest::_checkAvailable(void)
{
    MissionItem missionItem(1,              // sequence number
                            MAV_CMD_NAV_TAKEOFF,
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,
                            10.1234567,     // param 1-7
                            20.1234567,
                            30.1234567,
                            40.1234567,
                            50.1234567,
                            60.1234567,
                            70.1234567,
                            true,           // autoContinue
                            false);         // isCurrentItem
    SimpleMissionItem* item = new SimpleMissionItem(_offlineVehicle, missionItem);
    QVERIFY(item->cameraSection());
    QCOMPARE(item->cameraSection()->available(), false);
}


void CameraSectionTest::_testItemCount(void)
{
    // No settings specified to start
    QCOMPARE(_cameraSection->itemCount(), 0);

    // Check specify methods

    _cameraSection->setSpecifyGimbal(true);
    QCOMPARE(_cameraSection->itemCount(), 1);
    QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 1);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyGimbal(false);
    QCOMPARE(_cameraSection->itemCount(), 0);
    QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 0);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyCameraMode(true);
    QCOMPARE(_cameraSection->itemCount(), 1);
    QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 1);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyCameraMode(false);
    QCOMPARE(_cameraSection->itemCount(), 0);
    QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 0);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyGimbal(true);
    _cameraSection->setSpecifyCameraMode(true);
    QCOMPARE(_cameraSection->itemCount(), 2);
    QVERIFY(_spySection->checkSignalsByMask(itemCountChangedMask));
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    _cameraSection->setSpecifyGimbal(false);
    _cameraSection->setSpecifyCameraMode(false);
    QCOMPARE(_cameraSection->itemCount(), 0);
    QVERIFY(_spySection->checkSignalsByMask(itemCountChangedMask));
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    // Check camera actions

    QList<int> rgCameraActions;
    rgCameraActions << CameraSection::TakePhotosIntervalTime << CameraSection::TakePhotoIntervalDistance << CameraSection::StopTakingPhotos << CameraSection::TakeVideo << CameraSection::StopTakingVideo;
    foreach(int cameraAction, rgCameraActions) {
        qDebug() << "camera action" << cameraAction;

        // Reset
        _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
        QCOMPARE(_cameraSection->itemCount(), 0);
        _spySection->clearAllSignals();
        _spyCamera->clearAllSignals();

        // Set to new action
        _cameraSection->cameraAction()->setRawValue(cameraAction);
        QCOMPARE(_cameraSection->itemCount(), 1);
        QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
        QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 1);
        _spySection->clearAllSignals();
        _spyCamera->clearAllSignals();
    }

    // Reset
    _cameraSection->setSpecifyGimbal(false);
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    QCOMPARE(_cameraSection->itemCount(), 0);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();

    // Check both camera action and gimbal set
    _cameraSection->setSpecifyGimbal(true);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();
    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    QCOMPARE(_cameraSection->itemCount(), 2);
    QVERIFY(_spySection->checkSignalsByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 2);
    _spySection->clearAllSignals();
    _spyCamera->clearAllSignals();
}


void CameraSectionTest::_testAppendSectionItems(void)
{
    int seqNum = 0;
    QList<MissionItem*> rgMissionItems;

    // No settings specified to start
    QCOMPARE(_cameraSection->itemCount(), 0);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 0);
    QCOMPARE(seqNum, 0);
    rgMissionItems.clear();

    // Test specifyGimbal

    _cameraSection->setSpecifyGimbal(true);
    _cameraSection->gimbalPitch()->setRawValue(_validGimbalItem->missionItem().param1());
    _cameraSection->gimbalYaw()->setRawValue(_validGimbalItem->missionItem().param3());
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validGimbalItem->missionItem());
    _cameraSection->setSpecifyGimbal(false);
    rgMissionItems.clear();
    seqNum = 0;

    // Test specifyCameraMode

    _cameraSection->setSpecifyCameraMode(true);
    _cameraSection->cameraMode()->setRawValue(0);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validCameraPhotoModeItem->missionItem());
    _cameraSection->setSpecifyGimbal(false);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->setSpecifyCameraMode(true);
    _cameraSection->cameraMode()->setRawValue(1);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validCameraVideoModeItem->missionItem());
    _cameraSection->setSpecifyCameraMode(false);
    rgMissionItems.clear();
    seqNum = 0;

    // Test camera actions

    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    _cameraSection->cameraPhotoIntervalTime()->setRawValue(_validTimeItem->missionItem().param2());
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validTimeItem->missionItem());
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotoIntervalDistance);
    _cameraSection->cameraPhotoIntervalDistance()->setRawValue(_validDistanceItem->missionItem().param1());
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validDistanceItem->missionItem());
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->cameraAction()->setRawValue(CameraSection::TakeVideo);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validStartVideoItem->missionItem());
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->cameraAction()->setRawValue(CameraSection::StopTakingVideo);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validStopVideoItem->missionItem());
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->cameraAction()->setRawValue(CameraSection::StopTakingPhotos);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 2);
    QCOMPARE(seqNum, 2);
    _missionItemsEqual(*rgMissionItems[0], _validStopDistanceItem->missionItem());
    _missionItemsEqual(*rgMissionItems[1], _validStopTimeItem->missionItem());
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    rgMissionItems.clear();
    seqNum = 0;

    // Test both
    _cameraSection->setSpecifyGimbal(true);
    _cameraSection->gimbalPitch()->setRawValue(_validGimbalItem->missionItem().param1());
    _cameraSection->gimbalYaw()->setRawValue(_validGimbalItem->missionItem().param3());
    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    _cameraSection->cameraPhotoIntervalTime()->setRawValue(_validTimeItem->missionItem().param2());
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 2);
    QCOMPARE(seqNum, 2);
    _missionItemsEqual(*rgMissionItems[0], _validGimbalItem->missionItem());
    _missionItemsEqual(*rgMissionItems[1], _validTimeItem->missionItem());
    _cameraSection->setSpecifyGimbal(false);
    rgMissionItems.clear();
    seqNum = 0;
}

void CameraSectionTest::_testScanForGimbalSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    // Check for a scan success

    SimpleMissionItem* newValidGimbalItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidGimbalItem->missionItem() = _validGimbalItem->missionItem();
    visualItems.append(newValidGimbalItem);
    scanIndex = 0;
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->specifyGimbal(), true);
    QCOMPARE(_cameraSection->gimbalPitch()->rawValue().toDouble(), _validGimbalItem->missionItem().param1());
    QCOMPARE(_cameraSection->gimbalYaw()->rawValue().toDouble(), _validGimbalItem->missionItem().param3());
    _cameraSection->setSpecifyGimbal(false);
    visualItems.clear();
    scanIndex = 0;

#if 0
    MAV_CMD_DO_MOUNT_CONTROL
    Mission Param #1	pitch (WIP: DEPRECATED: or lat in degrees) depending on mount mode.
    Mission Param #2	roll (WIP: DEPRECATED: or lon in degrees) depending on mount mode.
    Mission Param #3	yaw (WIP: DEPRECATED: or alt in meters) depending on mount mode.
    Mission Param #4	WIP: alt in meters depending on mount mode.
    Mission Param #5	WIP: latitude in degrees * 1E7, set if appropriate mount mode.
    Mission Param #6	WIP: longitude in degrees * 1E7, set if appropriate mount mode.
    Mission Param #7	MAV_MOUNT_MODE enum value
#endif

    // Gimbal command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_offlineVehicle, _validGimbalItem->missionItem());
    invalidSimpleItem.missionItem().setParam2(10);    // roll is not supported
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam4(10);    // alt is not supported
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam5(10);    // lat is not supported
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam6(10);    // lon is not supported
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam7(MAV_MOUNT_MODE_RETRACT);    // Only MAV_MOUNT_MODE_MAVLINK_TARGETING supported
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForCameraModeSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    // Check for a scan success

    SimpleMissionItem* newValidCameraModeItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidCameraModeItem->missionItem() = _validCameraPhotoModeItem->missionItem();
    visualItems.append(newValidCameraModeItem);
    scanIndex = 0;
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->specifyCameraMode(), true);
    QCOMPARE(_cameraSection->cameraMode()->rawValue().toDouble(), _validCameraPhotoModeItem->missionItem().param2());
    _cameraSection->setSpecifyCameraMode(false);
    visualItems.clear();
    scanIndex = 0;

    newValidCameraModeItem->missionItem() = _validCameraVideoModeItem->missionItem();
    visualItems.append(newValidCameraModeItem);
    scanIndex = 0;
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->specifyCameraMode(), true);
    QCOMPARE(_cameraSection->cameraMode()->rawValue().toDouble(), _validCameraVideoModeItem->missionItem().param2());
    _cameraSection->setSpecifyCameraMode(false);
    visualItems.clear();
    scanIndex = 0;

#if 0
    MAV_CMD_SET_CAMERA_MODE
    Mission Param #1	Camera ID (0 for all cameras, 1 for first, 2 for second, etc.)
    Mission Param #2	Camera mode (0: photo mode, 1: video mode)
    Mission Param #3	Audio recording enabled (0: off 1: on)
    Mission Param #4	Reserved (all remaining params)
#endif

    // Mode command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_offlineVehicle, _validCameraPhotoModeItem->missionItem());
    invalidSimpleItem.missionItem().setParam3(0);   // Audio is not supported
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyCameraMode(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForPhotoIntervalTimeSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

#if 0
    MAV_CMD_IMAGE_START_CAPTURE	Start image capture sequence. Sends CAMERA_IMAGE_CAPTURED after each capture.
    Mission Param #1	Duration between two consecutive pictures (in seconds)
    Mission Param #2	Number of images to capture total - 0 for unlimited capture
    Mission Param #3	Resolution in megapixels (0.3 for 640x480, 1.3 for 1280x720, etc), set to 0 if param 4/5 are used, set to -1 for highest resolution possible.
    Mission Param #4	WIP: Resolution horizontal in pixels
    Mission Param #5	WIP: Resolution horizontal in pixels
    Mission Param #6	WIP: Camera ID    // Check for a scan success
#endif

    SimpleMissionItem* newValidTimeItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidTimeItem->missionItem() = _validTimeItem->missionItem();
    visualItems.append(newValidTimeItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakePhotosIntervalTime);
    QCOMPARE(_cameraSection->cameraPhotoIntervalTime()->rawValue().toInt(), (int)_validTimeItem->missionItem().param1());
    visualItems.clear();
    scanIndex = 0;

    // Image start command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_offlineVehicle, _validTimeItem->missionItem());
    invalidSimpleItem.missionItem().setParam3(10);    // must be unlimited
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validTimeItem->missionItem();
    invalidSimpleItem.missionItem().setParam3(1.3);    // must be -1
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validTimeItem->missionItem();
    invalidSimpleItem.missionItem().setParam4(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validTimeItem->missionItem();
    invalidSimpleItem.missionItem().setParam5(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validTimeItem->missionItem();
    invalidSimpleItem.missionItem().setParam6(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validTimeItem->missionItem();
    invalidSimpleItem.missionItem().setParam7(10);      // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForPhotoIntervalDistanceSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

#if 0
    MAV_CMD_DO_SET_CAM_TRIGG_DIST	Mission command to set CAM_TRIGG_DIST for this flight
    Mission Param #1	Camera trigger distance (meters)
    Mission Param #2	Empty
    Mission Param #3	Empty
    Mission Param #4	Empty
    Mission Param #5	Empty
    Mission Param #6	Empty
    Mission Param #7	Empty
#endif

    SimpleMissionItem* newValidDistanceItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidDistanceItem->missionItem() = _validDistanceItem->missionItem();
    visualItems.append(newValidDistanceItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakePhotoIntervalDistance);
    QCOMPARE(_cameraSection->cameraPhotoIntervalDistance()->rawValue().toInt(), (int)_validDistanceItem->missionItem().param1());
    visualItems.clear();
    scanIndex = 0;

    // Trigger distance command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_offlineVehicle, _validDistanceItem->missionItem());
    invalidSimpleItem.missionItem().setParam1(-1);    // must be >= 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validDistanceItem->missionItem();
    invalidSimpleItem.missionItem().setParam2(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validDistanceItem->missionItem();
    invalidSimpleItem.missionItem().setParam3(1);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validDistanceItem->missionItem();
    invalidSimpleItem.missionItem().setParam4(100);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validDistanceItem->missionItem();
    invalidSimpleItem.missionItem().setParam5(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validDistanceItem->missionItem();
    invalidSimpleItem.missionItem().setParam6(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validDistanceItem->missionItem();
    invalidSimpleItem.missionItem().setParam7(10);      // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForStartVideoSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

#if 0
    MAV_CMD_VIDEO_STOP_CAPTURE	Stop the current video capture (recording)
    Mission Param #1	WIP: Camera ID
    Mission Param #2	Reserved
    Mission Param #3	Resolution in megapixels (0.3 for 640x480, 1.3 for 1280x720, etc), set to 0 if param 4/5 are used, set to -1 for highest resolution possible.
    Mission Param #4	WIP: Resolution horizontal in pixels
    Mission Param #5	WIP: Resolution horizontal in pixels
    Mission Param #6	WIP: Frequency CAMERA_CAPTURE_STATUS messages should be sent while recording (0 for no messages, otherwise time in Hz)
#endif

    SimpleMissionItem* newValidStartVideoItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidStartVideoItem->missionItem() = _validStartVideoItem->missionItem();
    visualItems.append(newValidStartVideoItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakeVideo);
    visualItems.clear();
    scanIndex = 0;

    // Trigger distance command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_offlineVehicle, _validStartVideoItem->missionItem());
    invalidSimpleItem.missionItem().setParam1(10);    // must be  0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam2(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam3(1);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam4(100);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam5(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam6(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam7(10);      // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForStopVideoSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

#if 0
    MAV_CMD_VIDEO_STOP_CAPTURE	Stop the current video capture (recording)
    Mission Param #1	WIP: Camera ID
#endif

    SimpleMissionItem* newValidStopVideoItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidStopVideoItem->missionItem() = _validStopVideoItem->missionItem();
    visualItems.append(newValidStopVideoItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::StopTakingVideo);
    visualItems.clear();
    scanIndex = 0;

    // Trigger distance command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_offlineVehicle, _validStopVideoItem->missionItem());
    invalidSimpleItem.missionItem().setParam1(10);    // must be  0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStopVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam2(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStopVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam3(1);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStopVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam4(100);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStopVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam5(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStopVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam6(10);    // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStopVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam7(10);      // must be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForStopImageSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    SimpleMissionItem* newValidStopDistanceItem = new SimpleMissionItem(_offlineVehicle, this);
    SimpleMissionItem* newValidStopTimeItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidStopDistanceItem->missionItem() = _validStopDistanceItem->missionItem();
    newValidStopTimeItem->missionItem() = _validStopTimeItem->missionItem();
    visualItems.append(newValidStopDistanceItem);
    visualItems.append(newValidStopTimeItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::StopTakingPhotos);
    visualItems.clear();

    // Out of order commands

    SimpleMissionItem validStopDistanceItem(_offlineVehicle);
    SimpleMissionItem validStopTimeItem(_offlineVehicle);
    validStopDistanceItem.missionItem() = _validStopDistanceItem->missionItem();
    validStopTimeItem.missionItem() = _validStopTimeItem->missionItem();
    visualItems.append(&validStopTimeItem);
    visualItems.append(&validStopDistanceItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 2);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForFullSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    SimpleMissionItem* newValidGimbalItem = new SimpleMissionItem(_offlineVehicle, this);
    SimpleMissionItem* newValidDistanceItem = new SimpleMissionItem(_offlineVehicle, this);
    newValidGimbalItem->missionItem() = _validGimbalItem->missionItem();
    newValidDistanceItem->missionItem() = _validDistanceItem->missionItem();
    visualItems.append(newValidGimbalItem);
    visualItems.append(newValidDistanceItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->specifyGimbal(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakePhotoIntervalDistance);
    QCOMPARE(_cameraSection->gimbalPitch()->rawValue().toDouble(), _validGimbalItem->missionItem().param1());
    QCOMPARE(_cameraSection->gimbalYaw()->rawValue().toDouble(), _validGimbalItem->missionItem().param3());
    QCOMPARE(_cameraSection->cameraPhotoIntervalDistance()->rawValue().toInt(), (int)_validDistanceItem->missionItem().param1());
    visualItems.clear();
}
