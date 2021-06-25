/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraSectionTest.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"

#include <functional>

CameraSectionTest::CameraSectionTest(void)
    : _spyCamera                        (nullptr)
    , _spySection                       (nullptr)
    , _cameraSection                    (nullptr)
    , _validGimbalItem                  (nullptr)
    , _validDistanceItem                (nullptr)
    , _validTimeItem                    (nullptr)
    , _validStartVideoItem              (nullptr)
    , _validCameraPhotoModeItem         (nullptr)
    , _validCameraVideoModeItem         (nullptr)
    , _validCameraSurveyPhotoModeItem   (nullptr)
{    
    rgCameraSignals[specifyGimbalChangedIndex] =        SIGNAL(specifyGimbalChanged(bool));
    rgCameraSignals[specifiedGimbalYawChangedIndex] =   SIGNAL(specifiedGimbalYawChanged(double));
    rgCameraSignals[specifiedGimbalPitchChangedIndex] = SIGNAL(specifiedGimbalPitchChanged(double));
    rgCameraSignals[specifyCameraModeChangedIndex] =    SIGNAL(specifyCameraModeChanged(bool));
}

void CameraSectionTest::init(void)
{
    SectionTest::init();

    _cameraSection = _simpleItem->cameraSection();
    _createSpy(_cameraSection, &_spyCamera);
    QVERIFY(_spyCamera);
    SectionTest::_createSpy(_cameraSection, &_spySection);
    QVERIFY(_spySection);

    _validGimbalItem = new SimpleMissionItem(_masterController,
                                             false, // flyView
                                             MissionItem(0,
                                                         MAV_CMD_DO_MOUNT_CONTROL,
                                                         MAV_FRAME_MISSION,
                                                         10.1234, 0, 20.1234,               // pitch, roll, yaw
                                                         0, 0, 0,                           // alt, lat, lon (all 0 since unused)
                                                         MAV_MOUNT_MODE_MAVLINK_TARGETING,  // control gimbal with pitch, roll, yaw settings
                                                         true, false));
    _validTimeItem = new SimpleMissionItem(_masterController,
                                           false, // flyView
                                           MissionItem(0,
                                                       MAV_CMD_IMAGE_START_CAPTURE,
                                                       MAV_FRAME_MISSION,
                                                       0,                           // Reserved, must be 0
                                                       48,                          // time interval
                                                       0,                           // 0 = capture forever
                                                       NAN, NAN, NAN, NAN,          // Reserved
                                                       true, false));
    _validDistanceItem = new SimpleMissionItem(_masterController,
                                               false, // flyView
                                               MissionItem(0,
                                                           MAV_CMD_DO_SET_CAM_TRIGG_DIST,
                                                           MAV_FRAME_MISSION,
                                                           72,              // trigger distance
                                                           0,               // not shutter integration
                                                           1,               // trigger immediately
                                                           0, 0, 0, 0,
                                                           true, false));
    _validStartVideoItem = new SimpleMissionItem(_masterController,
                                                 false, // flyView
                                                 MissionItem(0,                             // sequence number
                                                             MAV_CMD_VIDEO_START_CAPTURE,
                                                             MAV_FRAME_MISSION,
                                                             0,                             // Reserved (Set to 0)
                                                             VIDEO_CAPTURE_STATUS_INTERVAL, // CAMERA_CAPTURE_STATUS (default to every 5 seconds)
                                                             NAN, NAN, NAN, NAN, NAN,       // param 3-7 reserved
                                                             true,                          // autocontinue
                                                             false));                       // isCurrentItem
    _validCameraPhotoModeItem = new SimpleMissionItem(_masterController,
                                                      false, // flyView
                                                      MissionItem(0,                               // sequence number
                                                                  MAV_CMD_SET_CAMERA_MODE,
                                                                  MAV_FRAME_MISSION,
                                                                  0,                               // Reserved (Set to 0)
                                                                  CAMERA_MODE_IMAGE,
                                                                  NAN, NAN, NAN, NAN, NAN,         // param 3-7 reserved
                                                                  true,                            // autocontinue
                                                                  false));                         // isCurrentItem
    _validCameraVideoModeItem = new SimpleMissionItem(_masterController,
                                                      false, // flyView
                                                      MissionItem(0,                               // sequence number
                                                                  MAV_CMD_SET_CAMERA_MODE,
                                                                  MAV_FRAME_MISSION,
                                                                  0,                               // Reserved (Set to 0)
                                                                  CAMERA_MODE_VIDEO,
                                                                  NAN, NAN, NAN, NAN, NAN,         // param 3-7 reserved
                                                                  true,                            // autocontinue
                                                                  false));                         // isCurrentItem
    _validCameraSurveyPhotoModeItem = new SimpleMissionItem(_masterController,
                                                            false, // flyView
                                                            MissionItem(0,                          // sequence number
                                                                        MAV_CMD_SET_CAMERA_MODE,
                                                                        MAV_FRAME_MISSION,
                                                                        0,                          // Reserved (Set to 0)
                                                                        CAMERA_MODE_IMAGE_SURVEY,
                                                                        NAN, NAN, NAN, NAN, NAN,    // param 3-7 reserved
                                                                        true,                       // autocontinue
                                                                        false));                    // isCurrentItem
    _validTakePhotoItem = new SimpleMissionItem(_masterController,
                                                false, // flyView
                                                MissionItem(0,
                                                            MAV_CMD_IMAGE_START_CAPTURE,
                                                            MAV_FRAME_MISSION,
                                                            0,                              // Reserved (Set to 0)
                                                            0,                              // Interval (none)
                                                            1,                              // Take 1 photo
                                                            0,                              // Sequence id not used
                                                            NAN, NAN, NAN,                  // param 5-7 reserved
                                                            true,                           // autoContinue
                                                            false));                        // isCurrentItem

    _validStopVideoItem =       createValidStopVideoItem(_masterController);
    _validStopDistanceItem =    createValidStopDistanceItem(_masterController);
    _validStopTimeItem =        createValidStopTimeItem(_masterController);
}

void CameraSectionTest::cleanup(void)
{
    delete _spyCamera;
    delete _spySection;

    _spyCamera      = nullptr;
    _spySection     = nullptr;
    _cameraSection  = nullptr;

    SectionTest::cleanup();

    // Deletion of _masterController will delete these obects
    _validGimbalItem                = nullptr;
    _validDistanceItem              = nullptr;
    _validTimeItem                  = nullptr;
    _validStartVideoItem            = nullptr;
    _validStopVideoItem             = nullptr;
    _validStopDistanceItem          = nullptr;
    _validStopTimeItem              = nullptr;
    _validTakePhotoItem             = nullptr;
    _validCameraPhotoModeItem       = nullptr;
    _validCameraVideoModeItem       = nullptr;
    _validCameraSurveyPhotoModeItem = nullptr;
}

void CameraSectionTest::_createSpy(CameraSection* cameraSection, MultiSignalSpy** cameraSpy)
{
    *cameraSpy = nullptr;
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

    // dirty SHOULD NOT change if pitch or yaw is changed while specifyGimbal IS NOT set
    _cameraSection->setSpecifyGimbal(false);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();
    _cameraSection->gimbalPitch()->setRawValue(_cameraSection->gimbalPitch()->rawValue().toDouble() + 1);
    _cameraSection->gimbalYaw()->setRawValue(_cameraSection->gimbalYaw()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkNoSignalByMask(dirtyChangedMask));
    QCOMPARE(_cameraSection->dirty(), false);

    // dirty SHOULD change if pitch or yaw is changed while specifyGimbal IS set
    _cameraSection->setSpecifyGimbal(true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();
    _cameraSection->gimbalPitch()->setRawValue(_cameraSection->gimbalPitch()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();
    _cameraSection->gimbalYaw()->setRawValue(_cameraSection->gimbalYaw()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_cameraSection->dirty(), true);
    _cameraSection->setDirty(false);
    _spySection->clearAllSignals();

    // Check the remaining items that should set dirty bit

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
    SimpleMissionItem* item = new SimpleMissionItem(_masterController, false /* flyView */, missionItem);
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
    rgCameraActions << CameraSection::TakePhotosIntervalTime << CameraSection::TakePhotoIntervalDistance << CameraSection::StopTakingPhotos << CameraSection::TakeVideo << CameraSection::StopTakingVideo << CameraSection::TakePhoto;
    for(int cameraAction: rgCameraActions) {
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

    _cameraSection->setSpecifyCameraMode(true);
    _cameraSection->cameraMode()->setRawValue(CAMERA_MODE_IMAGE);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validCameraPhotoModeItem->missionItem());
    _cameraSection->setSpecifyGimbal(false);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->setSpecifyCameraMode(true);
    _cameraSection->cameraMode()->setRawValue(CAMERA_MODE_VIDEO);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validCameraVideoModeItem->missionItem());
    _cameraSection->setSpecifyCameraMode(false);
    rgMissionItems.clear();
    seqNum = 0;

    _cameraSection->setSpecifyCameraMode(true);
    _cameraSection->cameraMode()->setRawValue(CAMERA_MODE_IMAGE_SURVEY);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validCameraSurveyPhotoModeItem->missionItem());
    _cameraSection->setSpecifyCameraMode(false);
    rgMissionItems.clear();
    seqNum = 0;

    // Test camera actions

    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhoto);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    _missionItemsEqual(*rgMissionItems[0], _validTakePhotoItem->missionItem());
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    rgMissionItems.clear();
    seqNum = 0;

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

    // Test multiple

    _cameraSection->setSpecifyGimbal(true);
    _cameraSection->gimbalPitch()->setRawValue(_validGimbalItem->missionItem().param1());
    _cameraSection->gimbalYaw()->setRawValue(_validGimbalItem->missionItem().param3());
    _cameraSection->cameraAction()->setRawValue(CameraSection::TakePhotosIntervalTime);
    _cameraSection->cameraPhotoIntervalTime()->setRawValue(_validTimeItem->missionItem().param2());
    _cameraSection->setSpecifyCameraMode(true);
    _cameraSection->cameraMode()->setRawValue(CAMERA_MODE_IMAGE);
    _cameraSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 3);
    QCOMPARE(seqNum, 3);
    _missionItemsEqual(*rgMissionItems[0], _validCameraPhotoModeItem->missionItem());   // Camera mode change must always be first
    _missionItemsEqual(*rgMissionItems[1], _validGimbalItem->missionItem());
    _missionItemsEqual(*rgMissionItems[2], _validTimeItem->missionItem());
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

    SimpleMissionItem* newValidGimbalItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
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

    /*
    MAV_CMD_DO_MOUNT_CONTROL
    Mission Param #1	pitch (WIP: DEPRECATED: or lat in degrees) depending on mount mode.
    Mission Param #2	roll (WIP: DEPRECATED: or lon in degrees) depending on mount mode.
    Mission Param #3	yaw (WIP: DEPRECATED: or alt in meters) depending on mount mode.
    Mission Param #4	WIP: alt in meters depending on mount mode.
    Mission Param #5	WIP: latitude in degrees * 1E7, set if appropriate mount mode.
    Mission Param #6	WIP: longitude in degrees * 1E7, set if appropriate mount mode.
    Mission Param #7	MAV_MOUNT_MODE enum value
    */

    // Gimbal command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validGimbalItem->missionItem());
    invalidSimpleItem.missionItem().setParam2(10);    // roll is not supported, should be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam4(10);    // alt is not supported, should be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam5(10);    // lat is not supported, should be 0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->specifyGimbal(), false);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validGimbalItem->missionItem();
    invalidSimpleItem.missionItem().setParam6(10);    // lon is not supported, should be 0
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

    SimpleMissionItem* newValidCameraModeItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
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

    /*
    MAV_CMD_SET_CAMERA_MODE
    Mission Param #1	Reserved (Set to 0)
    Mission Param #2	Camera mode (0: photo mode, 1: video mode)
    Mission Param #3	Reserved (all remaining params)
    */

    // Mode command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validCameraPhotoModeItem->missionItem());
    std::function<void(MissionItem&, double)> rgSetParamFns[] = {
            &MissionItem::setParam1,
            &MissionItem::setParam2,
            &MissionItem::setParam3,
            &MissionItem::setParam4,
            &MissionItem::setParam5,
            &MissionItem::setParam6,
            &MissionItem::setParam7
    };

    for (int fnIndex=2; fnIndex<7; fnIndex++) {
        rgSetParamFns[fnIndex](invalidSimpleItem.missionItem(), 0); // should be NaN
        visualItems.append(&invalidSimpleItem);
        QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
        QCOMPARE(visualItems.count(), 1);
        QCOMPARE(_cameraSection->specifyCameraMode(), false);
        QCOMPARE(_cameraSection->settingsSpecified(), false);
        visualItems.clear();
    }
}

void CameraSectionTest::_testScanForPhotoIntervalTimeSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    /*
    MAV_CMD_IMAGE_START_CAPTURE	WIP: Start image capture sequence. Sends CAMERA_IMAGE_CAPTURED after each capture.
    Mission Param #1	Reserved (Set to 0)
    Mission Param #2	Duration between two consecutive pictures (in seconds)
    Mission Param #3	Number of images to capture total - 0 for unlimited capture
*/

    SimpleMissionItem* newValidTimeItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
    newValidTimeItem->missionItem() = _validTimeItem->missionItem();
    visualItems.append(newValidTimeItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakePhotosIntervalTime);
    QCOMPARE(_cameraSection->cameraPhotoIntervalTime()->rawValue().toInt(), (int)_validTimeItem->missionItem().param2());
    visualItems.clear();
    scanIndex = 0;

    // Image start command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validTimeItem->missionItem());
    invalidSimpleItem.missionItem().setParam3(10);    // must be 0 for unlimited
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    visualItems.clear();
}

void CameraSectionTest::_testScanForPhotoIntervalDistanceSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    /*
    MAV_CMD_DO_SET_CAM_TRIGG_DIST	Mission command to set camera trigger distance for this flight. The camera is trigerred each time this distance is exceeded. This command can also be used to set the shutter integration time for the camera.
    Mission Param #1	Camera trigger distance (meters). 0 to stop triggering.
    Mission Param #2	Camera shutter integration time (milliseconds). -1 or 0 to ignore
    Mission Param #3	Trigger camera once immediately. (0 = no trigger, 1 = trigger)
    Mission Param #4	Empty
    Mission Param #5	Empty
    Mission Param #6	Empty
    Mission Param #7	Empty
    */

    SimpleMissionItem* newValidDistanceItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
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

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validDistanceItem->missionItem());
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
    invalidSimpleItem.missionItem().setParam3(0);    // must be 1
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

    /*
    MAV_CMD_VIDEO_START_CAPTURE	WIP: Starts video capture (recording)
    Mission Param #1	Reserved (Set to 0)
    Mission Param #2	Frequency CAMERA_CAPTURE_STATUS messages should be sent while recording (0 for no messages, otherwise frequency in Hz)
    Mission Param #3	Reserved
    */

    SimpleMissionItem* newValidStartVideoItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
    newValidStartVideoItem->missionItem() = _validStartVideoItem->missionItem();
    visualItems.append(newValidStartVideoItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakeVideo);
    visualItems.clear();
    scanIndex = 0;

    // Start Video command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validStartVideoItem->missionItem());
    invalidSimpleItem.missionItem().setParam1(10);    // Reserved (must be 0)
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

    /*
    MAV_CMD_VIDEO_STOP_CAPTURE	Stop the current video capture (recording)
    Mission Param #1 Reserved (Set to 0)
    */

    SimpleMissionItem* newValidStopVideoItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
    newValidStopVideoItem->missionItem() = _validStopVideoItem->missionItem();
    visualItems.append(newValidStopVideoItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::StopTakingVideo);
    visualItems.clear();
    scanIndex = 0;

    // Trigger distance command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validStopVideoItem->missionItem());
    invalidSimpleItem.missionItem().setParam1(10);    // must be  0
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();

    invalidSimpleItem.missionItem() = _validStartVideoItem->missionItem();
    invalidSimpleItem.missionItem().setParam2(VIDEO_CAPTURE_STATUS_INTERVAL + 1);    // must be VIDEO_CAPTURE_STATUS_INTERVAL
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForStopPhotoSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    SimpleMissionItem* newValidStopDistanceItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
    SimpleMissionItem* newValidStopTimeItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
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

    SimpleMissionItem validStopDistanceItem(_masterController, false /* flyView */, false /* forLoad */);
    SimpleMissionItem validStopTimeItem(_masterController, false /* flyView */, false /* forLoad */);
    validStopDistanceItem.missionItem() = _validStopDistanceItem->missionItem();
    validStopTimeItem.missionItem() = _validStopTimeItem->missionItem();
    visualItems.append(&validStopTimeItem);
    visualItems.append(&validStopDistanceItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 2);
    QCOMPARE(_cameraSection->settingsSpecified(), false);
    visualItems.clear();
}

void CameraSectionTest::_testScanForTakePhotoSection(void)
{
    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    /*
    MAV_CMD_IMAGE_START_CAPTURE	WIP: Start image capture sequence. Sends CAMERA_IMAGE_CAPTURED after each capture.
    Mission Param #1	Reserved (Set to 0)
    Mission Param #2	Duration between two consecutive pictures (in seconds)
    Mission Param #3	Number of images to capture total - 0 for unlimited capture
    Mission Param #4	0 Unused sequence id
    */

    SimpleMissionItem* newValidTakePhotoItem = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
    newValidTakePhotoItem->missionItem() = _validTakePhotoItem->missionItem();
    visualItems.append(newValidTakePhotoItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_cameraSection->settingsSpecified(), true);
    QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakePhoto);
    visualItems.clear();
    scanIndex = 0;

    // Take Photo command but incorrect settings

    SimpleMissionItem invalidSimpleItem(_masterController, false /* flyView */, _validTimeItem->missionItem());
    invalidSimpleItem.missionItem().setParam3(10);    // must be 1 for single photo
    visualItems.append(&invalidSimpleItem);
    QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    visualItems.clear();
}

void CameraSectionTest::_validateItemScan(SimpleMissionItem* validItem)
{
    QVERIFY(_cameraSection->settingsSpecified());
    if (validItem == _validGimbalItem) {
        QCOMPARE(_cameraSection->specifyGimbal(), true);
        QCOMPARE(_cameraSection->gimbalPitch()->rawValue().toDouble(), validItem->missionItem().param1());
        QCOMPARE(_cameraSection->gimbalYaw()->rawValue().toDouble(), validItem->missionItem().param3());
    } else if (validItem == _validDistanceItem) {
        QCOMPARE(_cameraSection->cameraAction()->rawValue().toInt(), (int)CameraSection::TakePhotoIntervalDistance);
        QCOMPARE(_cameraSection->cameraPhotoIntervalDistance()->rawValue().toInt(), (int)_validDistanceItem->missionItem().param1());
    } else if (validItem == _validTimeItem) {
    } else if (validItem == _validStartVideoItem) {
    } else if (validItem == _validStopVideoItem) {
    } else if (validItem == _validTakePhotoItem) {
    } else if (validItem == _validCameraPhotoModeItem) {
    } else if (validItem == _validCameraVideoModeItem) {
    }
}

void CameraSectionTest::_resetSection(void)
{
    _cameraSection->gimbalYaw()->setRawValue(0);
    _cameraSection->gimbalPitch()->setRawValue(0);
    _cameraSection->setSpecifyGimbal(false);
    _cameraSection->cameraPhotoIntervalTime()->setRawValue(0);
    _cameraSection->cameraPhotoIntervalDistance()->setRawValue(0);
    _cameraSection->cameraAction()->setRawValue(CameraSection::CameraActionNone);
    _cameraSection->cameraMode()->setRawValue(CAMERA_MODE_IMAGE);
    _cameraSection->setSpecifyCameraMode(false);
}

/// Test that we can scan the commands associated with the camera section in various orders/combinations.
void CameraSectionTest::_testScanForMultipleItems(void)
{
    MissionCommandTree* commandTree = qgcApp()->toolbox()->missionCommandTree();

    QCOMPARE(_cameraSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_cameraSection);

    QList<SimpleMissionItem*> rgCameraItems;
    rgCameraItems << _validGimbalItem << _validCameraPhotoModeItem << _validCameraVideoModeItem;

    QList<SimpleMissionItem*> rgActionItems;
    rgActionItems << _validDistanceItem << _validTimeItem <<  _validStartVideoItem <<  _validStopVideoItem << _validTakePhotoItem;

    // Camera action followed by gimbal/mode
    for (SimpleMissionItem* actionItem: rgActionItems) {
        for (SimpleMissionItem* cameraItem: rgCameraItems) {
            SimpleMissionItem* item1 = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
            item1->missionItem() = actionItem->missionItem();
            SimpleMissionItem* item2 = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
            item2->missionItem() = cameraItem->missionItem();
            visualItems.append(item1);
            visualItems.append(item2);
            //qDebug() << commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, (MAV_CMD)item1->command())->rawName() << commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, (MAV_CMD)item2->command())->rawName();

            scanIndex = 0;
            QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);

            _validateItemScan(cameraItem);

            _resetSection();
            visualItems.clearAndDeleteContents();
        }
    }

    // Gimbal/Mode followed by camera action
    for (SimpleMissionItem* actionItem: rgCameraItems) {
        for (SimpleMissionItem* cameraItem: rgActionItems) {
            SimpleMissionItem* item1 = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
            item1->missionItem() = actionItem->missionItem();
            SimpleMissionItem* item2 = new SimpleMissionItem(_masterController, false /* flyView */, false /* forLoad */);
            item2->missionItem() = cameraItem->missionItem();
            visualItems.append(item1);
            visualItems.append(item2);
            qDebug() << commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, (MAV_CMD)item1->command())->rawName() << commandTree->getUIInfo(_controllerVehicle, QGCMAVLink::VehicleClassGeneric, (MAV_CMD)item2->command())->rawName();;

            scanIndex = 0;
            QCOMPARE(_cameraSection->scanForSection(&visualItems, scanIndex), true);

            _validateItemScan(cameraItem);

            _resetSection();
            visualItems.clearAndDeleteContents();
        }
    }
}

void CameraSectionTest::_testSpecifiedGimbalValuesChanged(void)
{
    // specifiedGimbal[Yaw|Pitch]Changed SHOULD NOT signal if values are changed when specifyGimbal IS NOT set
    _cameraSection->setSpecifyGimbal(false);
    _spyCamera->clearAllSignals();
    _cameraSection->gimbalYaw()->setRawValue(_cameraSection->gimbalYaw()->rawValue().toDouble() + 1);
    QVERIFY(_spyCamera->checkNoSignalByMask(specifiedGimbalYawChangedMask));
    _cameraSection->gimbalPitch()->setRawValue(_cameraSection->gimbalPitch()->rawValue().toDouble() + 1);
    QVERIFY(_spyCamera->checkNoSignalByMask(specifiedGimbalPitchChangedMask));

    // specifiedGimbal[Yaw|Pitch]Changed SHOULD signal if values are changed when specifyGimbal IS set
    _cameraSection->setSpecifyGimbal(true);
    _spyCamera->clearAllSignals();
    _cameraSection->gimbalYaw()->setRawValue(_cameraSection->gimbalYaw()->rawValue().toDouble() + 1);
    QVERIFY(_spyCamera->checkSignalByMask(specifiedGimbalYawChangedMask));
    _spyCamera->clearAllSignals();
    _cameraSection->gimbalPitch()->setRawValue(_cameraSection->gimbalPitch()->rawValue().toDouble() + 1);
    QVERIFY(_spyCamera->checkSignalByMask(specifiedGimbalPitchChangedMask));
}

SimpleMissionItem* CameraSectionTest::createValidStopVideoItem(PlanMasterController* masterController)
{
    return new SimpleMissionItem(masterController,
                                 false, // flyView
                                 MissionItem(0, MAV_CMD_VIDEO_STOP_CAPTURE, MAV_FRAME_MISSION, 0, qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), true, false));
}


SimpleMissionItem* CameraSectionTest::createValidStopDistanceItem(PlanMasterController* masterController)
{
    return new SimpleMissionItem(masterController,
                                 false, // flyView
                                 MissionItem(0, MAV_CMD_DO_SET_CAM_TRIGG_DIST, MAV_FRAME_MISSION, 0, 0, 0, 0, 0, 0, 0, true, false));
}

SimpleMissionItem* CameraSectionTest::createValidStopTimeItem(PlanMasterController* masterController)
{
    return new SimpleMissionItem(masterController,
                                 false, // flyView
                                 MissionItem(1, MAV_CMD_IMAGE_STOP_CAPTURE, MAV_FRAME_MISSION, 0, qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), qQNaN(), true, false));
}

SimpleMissionItem* CameraSectionTest::createInvalidStopVideoItem(PlanMasterController* masterController)
{
    SimpleMissionItem* invalidSimpleItem = createValidStopVideoItem(masterController);
    invalidSimpleItem->missionItem().setParam1(10);    // must be  0 to be valid for scan
    return invalidSimpleItem;
}


SimpleMissionItem* CameraSectionTest::createInvalidStopDistanceItem(PlanMasterController* masterController)
{
    SimpleMissionItem* invalidSimpleItem = createValidStopDistanceItem(masterController);
    invalidSimpleItem->missionItem().setParam2(-1);    // Should be 0
    return invalidSimpleItem;
}

SimpleMissionItem* CameraSectionTest::createInvalidStopTimeItem(PlanMasterController* masterController)
{
    SimpleMissionItem* invalidSimpleItem = createValidStopTimeItem(masterController);
    invalidSimpleItem->missionItem().setParam1(1);    // Should be 0
    return invalidSimpleItem;
}
