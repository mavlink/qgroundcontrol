#include "MissionControllerTest.h"
#include "MissionController.h"
#include "PlanMasterController.h"
#include "SimpleMissionItem.h"
#include "MissionSettingsItem.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "PlanViewSettings.h"
#include "MultiSignalSpy.h"

#include <QtTest/QTest>

void MissionControllerTest::cleanup()
{
    delete _masterController;
    delete _multiSpyMissionController;
    delete _multiSpyMissionItem;

    _masterController           = nullptr;
    _missionController          = nullptr;
    _multiSpyMissionController  = nullptr;
    _multiSpyMissionItem        = nullptr;

    MissionControllerManagerTest::cleanup();
}

void MissionControllerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    MissionControllerManagerTest::_initForFirmwareType(firmwareType);

    // Master controller pulls offline vehicle info from settings
    SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(firmwareType));
    _masterController = new PlanMasterController(this);
    _masterController->setFlyView(false);
    _missionController = _masterController->missionController();

    _multiSpyMissionController = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyMissionController);
    QCOMPARE(_multiSpyMissionController->init(_missionController), true);

    _masterController->start();

    // visualItemsChanged should come through on start
    QCOMPARE(_multiSpyMissionController->checkSignalsByMask(_multiSpyMissionController->mask("visualItemsChanged")), true);
    _multiSpyMissionController->clearAllSignals();

    QmlObjectListModel* visualItems = _missionController->visualItems();
    VERIFY_NOT_NULL(visualItems);

    // Empty vehicle only has home position
    QCOMPARE(visualItems->count(), 1);

    // Mission Settings should be in first slot
    MissionSettingsItem* settingsItem = visualItems->value<MissionSettingsItem*>(0);
    VERIFY_NOT_NULL(settingsItem);

    // Offline vehicle, so no home position
    QCOMPARE(settingsItem->coordinate().isValid(), false);

    // Empty mission, so no child items possible
    QCOMPARE(settingsItem->childItems()->count(), 0);

    // No waypoint lines
    QmlObjectListModel* simpleFlightPathSegments = _missionController->simpleFlightPathSegments();
    VERIFY_NOT_NULL(simpleFlightPathSegments);
    QCOMPARE(simpleFlightPathSegments->count(), 0);
}

void MissionControllerTest::_testEmptyVehicleWorker(MAV_AUTOPILOT firmwareType)
{
    _initForFirmwareType(firmwareType);

    // FYI: A significant amount of empty vehicle testing is in _initForFirmwareType since that
    // sets up an empty vehicle

    QmlObjectListModel* visualItems = _missionController->visualItems();
    VERIFY_NOT_NULL(visualItems);
    VisualMissionItem* visualItem = visualItems->value<VisualMissionItem*>(0);
    VERIFY_NOT_NULL(visualItem);

    _setupVisualItemSignals(visualItem);
}

void MissionControllerTest::_testEmptyVehiclePX4()
{
    _testEmptyVehicleWorker(MAV_AUTOPILOT_PX4);
}

void MissionControllerTest::_testEmptyVehicleAPM()
{
    _testEmptyVehicleWorker(MAV_AUTOPILOT_ARDUPILOTMEGA);
}

void MissionControllerTest::_testOfflineToOnlineWorker(MAV_AUTOPILOT firmwareType)
{
    // Start offline and add an item
    SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(firmwareType));
    _masterController = new PlanMasterController(this);
    _masterController->setFlyView(false);
    _missionController = _masterController->missionController();
    _masterController->start();

    // Add a waypoint in offline mode
    _missionController->insertSimpleMissionItem(QGeoCoordinate(37.803784, -122.462276), _missionController->visualItems()->count());

    // Verify we have 2 items (settings + waypoint)
    QCOMPARE(_missionController->visualItems()->count(), 2);

    // Now go online by connecting a vehicle
    MissionControllerManagerTest::_initForFirmwareType(firmwareType);

    // Offline items should be preserved when connecting to an empty vehicle
    QCOMPARE(_missionController->visualItems()->count(), 2);
}

void MissionControllerTest::_testOfflineToOnlineAPM()
{
    _testOfflineToOnlineWorker(MAV_AUTOPILOT_ARDUPILOTMEGA);
}

void MissionControllerTest::_testOfflineToOnlinePX4()
{
    _testOfflineToOnlineWorker(MAV_AUTOPILOT_PX4);
}

void MissionControllerTest::_setupVisualItemSignals(VisualMissionItem* visualItem)
{
    delete _multiSpyMissionItem;

    _multiSpyMissionItem = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyMissionItem);
    QCOMPARE(_multiSpyMissionItem->init(visualItem), true);
}

void MissionControllerTest::_testGimbalRecalc()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 1);
    _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 2);
    _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 3);
    _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 4);

    // No specific gimbal yaw set yet
    for (int i=1; i<_missionController->visualItems()->count(); i++) {
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        QVERIFY(qIsNaN(visualItem->missionGimbalYaw()));
    }

    // Specify gimbal yaw on settings item should generate yaw on all subsequent items
    const int yawIndex = 2;
    SimpleMissionItem* item = _missionController->visualItems()->value<SimpleMissionItem*>(yawIndex);
    item->cameraSection()->setSpecifyGimbal(true);
    item->cameraSection()->gimbalYaw()->setRawValue(0.0);
    SettingsManager::instance()->planViewSettings()->showGimbalOnlyWhenSet()->setRawValue(false);
    // Wait for recalc to complete - gimbal yaw should be set on item at yawIndex
    QVERIFY(TestHelpers::waitFor([&]() {
        VisualMissionItem* vi = _missionController->visualItems()->value<VisualMissionItem*>(yawIndex);
        return vi && !qIsNaN(vi->missionGimbalYaw());
    }));
    for (int i=1; i<_missionController->visualItems()->count(); i++) {
        //qDebug() << i;
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        if (i >= yawIndex) {
            QCOMPARE(visualItem->missionGimbalYaw(), 0.0);
        } else {
            QVERIFY(qIsNaN(visualItem->missionGimbalYaw()));
        }
    }
}

void MissionControllerTest::_testVehicleYawRecalc()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    double wpDistance   = 1000;
    double wpAngleInc   = 45;
    double wpAngle      = 0;

    int cMissionItems = 4;
    QGeoCoordinate currentCoord(0, 0);
    _missionController->insertSimpleMissionItem(currentCoord, 1);
    for (int i=2; i<=cMissionItems; i++) {
        wpAngle += wpAngleInc;
        currentCoord = currentCoord.atDistanceAndAzimuth(wpDistance, wpAngle);
        _missionController->insertSimpleMissionItem(currentCoord, i);
    }

    // Wait for recalc to complete - vehicle yaw should be set on item 2
    QVERIFY(TestHelpers::waitFor([&]() {
        VisualMissionItem* vi = _missionController->visualItems()->value<VisualMissionItem*>(2);
        return vi && !qIsNaN(vi->missionVehicleYaw());
    }));

    // No specific vehicle yaw set yet. Vehicle yaw should track flight path.
    double expectedVehicleYaw = wpAngleInc;
    for (int i=2; i<cMissionItems; i++) {
        //qDebug() << i;
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        QCOMPARE(visualItem->missionVehicleYaw(), expectedVehicleYaw);
        if (i <= cMissionItems - 1) {
            expectedVehicleYaw += wpAngleInc;
        }
    }

    SimpleMissionItem* simpleItem = _missionController->visualItems()->value<SimpleMissionItem*>(3);
    simpleItem->missionItem().setParam4(66);

    // Wait for recalc to complete - item 3's yaw should now be 66
    QVERIFY(TestHelpers::waitFor([&]() {
        VisualMissionItem* vi = _missionController->visualItems()->value<VisualMissionItem*>(3);
        return vi && vi->missionVehicleYaw() == 66.0;
    }));

    // All item should track vehicle path except for the one changed
    expectedVehicleYaw = wpAngleInc;
    for (int i=2; i<cMissionItems; i++) {
        //qDebug() << i;
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        QCOMPARE(visualItem->missionVehicleYaw(), i == 3 ? 66.0 : expectedVehicleYaw);
        if (i <= cMissionItems - 1) {
            expectedVehicleYaw += wpAngleInc;
        }
    }
}

void MissionControllerTest::_testLoadJsonSectionAvailable()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _masterController->loadFromFile(":/unittest/SectionTest.plan");

    QmlObjectListModel* visualItems = _missionController->visualItems();
    VERIFY_NOT_NULL(visualItems);
    QCOMPARE(visualItems->count(), 5);

    // Check that only waypoint items have camera and speed sections
    for (int i=1; i<visualItems->count(); i++) {
        SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(i);
        VERIFY_NOT_NULL(item);
        if ((int)item->command() == MAV_CMD_NAV_WAYPOINT) {
            QCOMPARE(item->cameraSection()->available(), true);
            QCOMPARE(item->speedSection()->available(), true);
        } else {
            QCOMPARE(item->cameraSection()->available(), false);
            QCOMPARE(item->speedSection()->available(), false);
        }

    }
}

void MissionControllerTest::_testGlobalAltMode()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    struct  _globalAltMode_s {
        QGroundControlQmlGlobal::AltMode   altMode;
        MAV_FRAME                               expectedMavFrame;
    } altModeTestCases[] = {
        { QGroundControlQmlGlobal::AltitudeModeRelative,            MAV_FRAME_GLOBAL_RELATIVE_ALT },
        { QGroundControlQmlGlobal::AltitudeModeAbsolute,            MAV_FRAME_GLOBAL },
        { QGroundControlQmlGlobal::AltitudeModeCalcAboveTerrain,    MAV_FRAME_GLOBAL },
        { QGroundControlQmlGlobal::AltitudeModeTerrainFrame,        MAV_FRAME_GLOBAL_TERRAIN_ALT },
    };

    for (const _globalAltMode_s& testCase: altModeTestCases) {
        _missionController->removeAll();
        _missionController->setGlobalAltitudeMode(testCase.altMode);

        _missionController->insertTakeoffItem(QGeoCoordinate(0, 0), 1);
        _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 2);
        _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 3);
        _missionController->insertSimpleMissionItem(QGeoCoordinate(0, 0), 4);

        SimpleMissionItem* si = qobject_cast<SimpleMissionItem*>(_missionController->visualItems()->value<VisualMissionItem*>(1));
        QCOMPARE(si->altitudeMode(), QGroundControlQmlGlobal::AltitudeModeRelative);
        QCOMPARE(si->missionItem().frame(), MAV_FRAME_GLOBAL_RELATIVE_ALT);

        for (int i=2; i<_missionController->visualItems()->count(); i++) {
            qDebug() << i;
            SimpleMissionItem* si = qobject_cast<SimpleMissionItem*>(_missionController->visualItems()->value<VisualMissionItem*>(i));
            QCOMPARE(si->altitudeMode(), testCase.altMode);
            QCOMPARE(si->missionItem().frame(), testCase.expectedMavFrame);
        }
    }
}
