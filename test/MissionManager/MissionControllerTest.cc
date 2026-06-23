#include "QmlObjectListModel.h"
#include "MissionControllerTest.h"

#include "AppSettings.h"
#include "CameraCalc.h"
#include "CorridorScanComplexItem.h"
#include "StructureScanComplexItem.h"
#include "SurveyComplexItem.h"
#include "UnitTestCoords.h"
#include "MissionController.h"
#include "MissionSettingsItem.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"
#include "SettingsManager.h"
#include "SimpleMissionItem.h"
#include "TestFixtures.h"
#include "MultiSignalSpy.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
using namespace TestFixtures;

MissionControllerTest::~MissionControllerTest() = default;

namespace {
// Coordinate checks involve geodesic reconstruction so they use 0.5 m. Altitude
// checks are direct arithmetic, so they use a much tighter 1e-4 m tolerance.
constexpr double kCoordToleranceMeters = 0.5;
constexpr double kAltToleranceMeters = 1e-4;
}  // namespace

void MissionControllerTest::cleanup()
{
    _masterController.reset();
    _missionController = nullptr;
    MissionControllerManagerTest::cleanup();
}

void MissionControllerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    MissionControllerManagerTest::_initForFirmwareType(firmwareType);
    // Master controller pulls offline vehicle info from settings
    SettingsManager::instance()->appSettings()->offlineEditingFirmwareClass()->setRawValue(
        QGCMAVLink::firmwareClass(firmwareType));
    _masterController = std::make_unique<PlanMasterController>();
    _masterController->setFlyView(false);
    _missionController = _masterController->missionController();
    MultiSignalSpy missionControllerSpy;
    QVERIFY(missionControllerSpy.init(_missionController));
    _masterController->start();
    // visualItemsReset should be emitted during start (along with many other signals)
    QVERIFY(missionControllerSpy.waitForSignal("visualItemsReset", TestTimeout::mediumMs()));
    QmlObjectListModel* visualItems = _missionController->visualItems();
    QVERIFY(visualItems);
    // Empty vehicle only has home position
    QCOMPARE(visualItems->count(), 1);
    // Mission Settings should be in first slot
    MissionSettingsItem* settingsItem = visualItems->value<MissionSettingsItem*>(0);
    QVERIFY(settingsItem);
    // Offline vehicle, so no home position
    QCOMPARE(settingsItem->coordinate().isValid(), false);
    // Empty mission, so no child items possible
    QCOMPARE(settingsItem->childItems()->count(), 0);
    // No waypoint lines
    QmlObjectListModel* simpleFlightPathSegments = _missionController->simpleFlightPathSegments();
    QVERIFY(simpleFlightPathSegments);
    QCOMPARE(simpleFlightPathSegments->count(), 0);
}

void MissionControllerTest::_testEmptyVehicle_data()
{
    TestData::addAutopilotRows();
}

void MissionControllerTest::_testEmptyVehicle()
{
    UT_FETCH_AUTOPILOT();
    Q_UNUSED(autopilotName);
    // ArduPilot mock link has no metadata source; these warnings are expected for that autopilot.
    ignoreLogMessage("ComponentInformation.RequestMetaDataTypeStateMachine", QtWarningMsg,
                     QRegularExpression("failed to load metadata"));
    ignoreLogMessage("FirmwarePlugin.ParameterMetaData", QtWarningMsg,
                     QRegularExpression("Skipping invalid enum value"));
    _initForFirmwareType(autopilot);
    // FYI: A significant amount of empty vehicle testing is in _initForFirmwareType since that
    // sets up an empty vehicle
    QmlObjectListModel* visualItems = _missionController->visualItems();
    QVERIFY(visualItems);
    VisualMissionItem* visualItem = visualItems->value<VisualMissionItem*>(0);
    QVERIFY(visualItem);
    _setupVisualItemSignals(visualItem);
}

void MissionControllerTest::_setupVisualItemSignals(VisualMissionItem* visualItem)
{
    MultiSignalSpy visualItemSpy;
    QVERIFY(visualItemSpy.init(visualItem));
}

void MissionControllerTest::_testInsertValidityHomePositionGating()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    auto boolProperty = [this](const char* name) {
        const QVariant value = _missionController->property(name);
        return value.isValid() && value.toBool();
    };

    // All insert validity properties must exist
    QVERIFY(_missionController->property("isInsertTakeoffValid").isValid());
    QVERIFY(_missionController->property("isInsertLandValid").isValid());
    QVERIFY(_missionController->property("isInsertROIValid").isValid());
    QVERIFY(_missionController->property("flyThroughCommandsAllowed").isValid());

    // No home position set: no inserts are valid
    QCOMPARE(_missionController->homePositionSet(), false);
    QCOMPARE(boolProperty("isInsertTakeoffValid"), false);
    QCOMPARE(boolProperty("isInsertLandValid"), false);
    QCOMPARE(boolProperty("isInsertROIValid"), false);
    QCOMPARE(boolProperty("flyThroughCommandsAllowed"), false);

    // Home position set, empty plan: only takeoff insert is valid
    _missionController->setHomePosition(Coord::zurich());
    QCOMPARE(_missionController->homePositionSet(), true);
    QCOMPARE(boolProperty("isInsertTakeoffValid"), true);
    QCOMPARE(boolProperty("isInsertLandValid"), false);
    QCOMPARE(boolProperty("isInsertROIValid"), false);
    QCOMPARE(boolProperty("flyThroughCommandsAllowed"), false);

    // Takeoff added: takeoff no longer valid, everything else is
    QVERIFY(_missionController->insertTakeoffItem(Coord::zurich(), 1, true /* makeCurrentItem */));
    QCOMPARE(boolProperty("isInsertTakeoffValid"), false);
    QCOMPARE(boolProperty("isInsertLandValid"), true);
    QCOMPARE(boolProperty("isInsertROIValid"), true);
    QCOMPARE(boolProperty("flyThroughCommandsAllowed"), true);
}

void MissionControllerTest::_testGimbalRecalc()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    // Use coordinate fixtures for test waypoints
    const QList<QGeoCoordinate> waypoints = Coord::waypointPath(Coord::zurich(), 4);
    for (int i = 0; i < waypoints.count(); ++i) {
        _missionController->insertSimpleMissionItem(waypoints[i], i + 1);
    }
    // No specific gimbal yaw set yet
    for (int i = 1; i < _missionController->visualItems()->count(); i++) {
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        QVERIFY(qIsNaN(visualItem->missionGimbalYaw()));
    }
    // Specify gimbal yaw on settings item should generate yaw on all subsequent items
    const int yawIndex = 2;
    SimpleMissionItem* item = _missionController->visualItems()->value<SimpleMissionItem*>(yawIndex);
    item->cameraSection()->setSpecifyGimbal(true);
    item->cameraSection()->gimbalYaw()->setRawValue(0.0);
    SettingsManager::instance()->planViewSettings()->showGimbalOnlyWhenSet()->setRawValue(false);
    QVERIFY_TRUE_WAIT(([&]() {
        for (int i = 1; i < _missionController->visualItems()->count(); i++) {
            VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
            if (i >= yawIndex) {
                if (!qFuzzyCompare(visualItem->missionGimbalYaw() + 1.0, 1.0)) {
                    return false;
                }
            } else if (!qIsNaN(visualItem->missionGimbalYaw())) {
                return false;
            }
        }
        return true;
    }()),
                      TestTimeout::mediumMs());
    for (int i = 1; i < _missionController->visualItems()->count(); i++) {
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
    double wpDistance = 1000;
    double wpAngleInc = 45;
    double wpAngle = 0;
    int cMissionItems = 4;
    QGeoCoordinate currentCoord(0, 0);
    _missionController->insertSimpleMissionItem(currentCoord, 1);
    for (int i = 2; i <= cMissionItems; i++) {
        wpAngle += wpAngleInc;
        currentCoord = currentCoord.atDistanceAndAzimuth(wpDistance, wpAngle);
        _missionController->insertSimpleMissionItem(currentCoord, i);
    }
    QVERIFY_TRUE_WAIT(([&]() {
        double expectedVehicleYaw = wpAngleInc;
        for (int i = 2; i < cMissionItems; i++) {
            VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
            if (!qFuzzyCompare(visualItem->missionVehicleYaw() + 1.0, expectedVehicleYaw + 1.0)) {
                return false;
            }
            if (i <= cMissionItems - 1) {
                expectedVehicleYaw += wpAngleInc;
            }
        }
        return true;
    }()),
                      TestTimeout::mediumMs());
    // No specific vehicle yaw set yet. Vehicle yaw should track flight path.
    double expectedVehicleYaw = wpAngleInc;
    for (int i = 2; i < cMissionItems; i++) {
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        QCOMPARE(visualItem->missionVehicleYaw(), expectedVehicleYaw);
        if (i <= cMissionItems - 1) {
            expectedVehicleYaw += wpAngleInc;
        }
    }
    SimpleMissionItem* simpleItem = _missionController->visualItems()->value<SimpleMissionItem*>(3);
    simpleItem->missionItem().setParam4(66);
    QVERIFY_TRUE_WAIT(([&]() {
        double expectedYaw = wpAngleInc;
        for (int i = 2; i < cMissionItems; i++) {
            VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
            const double expected = (i == 3) ? 66.0 : expectedYaw;
            if (!qFuzzyCompare(visualItem->missionVehicleYaw() + 1.0, expected + 1.0)) {
                return false;
            }
            if (i <= cMissionItems - 1) {
                expectedYaw += wpAngleInc;
            }
        }
        return true;
    }()),
                      TestTimeout::mediumMs());
    // All item should track vehicle path except for the one changed
    expectedVehicleYaw = wpAngleInc;
    for (int i = 2; i < cMissionItems; i++) {
        VisualMissionItem* visualItem = _missionController->visualItems()->value<VisualMissionItem*>(i);
        QCOMPARE(visualItem->missionVehicleYaw(), i == 3 ? 66.0 : expectedVehicleYaw);
        if (i <= cMissionItems - 1) {
            expectedVehicleYaw += wpAngleInc;
        }
    }
}

void MissionControllerTest::_testMissionReposition()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    MissionSettingsItem* settingsItem = _missionController->visualItems()->value<MissionSettingsItem*>(0);
    QVERIFY(settingsItem);

    const QGeoCoordinate home = Coord::zurich();
    settingsItem->setCoordinate(home);

    const QGeoCoordinate wp1 = home.atDistanceAndAzimuth(150.0, 15.0);
    const QGeoCoordinate wp2 = home.atDistanceAndAzimuth(320.0, 120.0);
    _missionController->insertSimpleMissionItem(wp1, 1);
    _missionController->insertSimpleMissionItem(wp2, 2);

    SimpleMissionItem* item1 = _missionController->visualItems()->value<SimpleMissionItem*>(1);
    SimpleMissionItem* item2 = _missionController->visualItems()->value<SimpleMissionItem*>(2);
    QVERIFY(item1);
    QVERIFY(item2);

    const double oldHomeAlt = settingsItem->editableAlt();
    const double oldAlt1 = item1->editableAlt();
    const double oldAlt2 = item2->editableAlt();

    const QGeoCoordinate newHome = home.atDistanceAndAzimuth(200.0, 65.0);
    const QGeoCoordinate expectedWp1 =
        newHome.atDistanceAndAzimuth(home.distanceTo(wp1), home.azimuthTo(wp1));
    const QGeoCoordinate expectedWp2 =
        newHome.atDistanceAndAzimuth(home.distanceTo(wp2), home.azimuthTo(wp2));
    _missionController->repositionMission(newHome, true, true);
    QVERIFY_TRUE_WAIT((settingsItem->coordinate().distanceTo(newHome) <= kCoordToleranceMeters) &&
                          (item1->coordinate().distanceTo(expectedWp1) <= kCoordToleranceMeters) &&
                          (item2->coordinate().distanceTo(expectedWp2) <= kCoordToleranceMeters),
                      TestTimeout::shortMs());

    QCOMPARE_COORDS(settingsItem->coordinate(), newHome, kCoordToleranceMeters);
    QCOMPARE_COORDS(item1->coordinate(), expectedWp1, kCoordToleranceMeters);
    QCOMPARE_COORDS(item2->coordinate(), expectedWp2, kCoordToleranceMeters);
    QCOMPARE_FUZZY(settingsItem->editableAlt(), oldHomeAlt, kAltToleranceMeters);
    QCOMPARE_FUZZY(item1->editableAlt(), oldAlt1, kAltToleranceMeters);
    QCOMPARE_FUZZY(item2->editableAlt(), oldAlt2, kAltToleranceMeters);
}

void MissionControllerTest::_testMissionOffset()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    MissionSettingsItem* settingsItem = _missionController->visualItems()->value<MissionSettingsItem*>(0);
    QVERIFY(settingsItem);

    const QGeoCoordinate home = Coord::zurich();
    settingsItem->setCoordinate(home);

    const QGeoCoordinate wp1 = home.atDistanceAndAzimuth(150.0, 15.0);
    const QGeoCoordinate wp2 = home.atDistanceAndAzimuth(320.0, 120.0);
    _missionController->insertSimpleMissionItem(wp1, 1);
    _missionController->insertSimpleMissionItem(wp2, 2);

    SimpleMissionItem* item1 = _missionController->visualItems()->value<SimpleMissionItem*>(1);
    SimpleMissionItem* item2 = _missionController->visualItems()->value<SimpleMissionItem*>(2);
    QVERIFY(item1);
    QVERIFY(item2);

    const double oldHomeAlt = settingsItem->editableAlt();
    const double oldAlt1 = item1->editableAlt();
    const double oldAlt2 = item2->editableAlt();

    _missionController->offsetMission(0.0, 0.0, 12.0, true, true);
    QVERIFY_TRUE_WAIT(qAbs(settingsItem->editableAlt() - oldHomeAlt) <= kAltToleranceMeters &&
                          qAbs(item1->editableAlt() - (oldAlt1 + 12.0)) <= kAltToleranceMeters &&
                          qAbs(item2->editableAlt() - (oldAlt2 + 12.0)) <= kAltToleranceMeters,
                      TestTimeout::shortMs());

    QCOMPARE_FUZZY(settingsItem->editableAlt(), oldHomeAlt, kAltToleranceMeters);
    QCOMPARE_FUZZY(item1->editableAlt(), oldAlt1 + 12.0, kAltToleranceMeters);
    QCOMPARE_FUZZY(item2->editableAlt(), oldAlt2 + 12.0, kAltToleranceMeters);
}

void MissionControllerTest::_testMissionRotate()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    MissionSettingsItem* settingsItem = _missionController->visualItems()->value<MissionSettingsItem*>(0);
    QVERIFY(settingsItem);

    const QGeoCoordinate home = Coord::zurich();
    settingsItem->setCoordinate(home);

    const QGeoCoordinate wp1 = home.atDistanceAndAzimuth(180.0, 20.0);
    const QGeoCoordinate wp2 = home.atDistanceAndAzimuth(260.0, 135.0);
    _missionController->insertSimpleMissionItem(wp1, 1);
    _missionController->insertSimpleMissionItem(wp2, 2);

    SimpleMissionItem* item1 = _missionController->visualItems()->value<SimpleMissionItem*>(1);
    SimpleMissionItem* item2 = _missionController->visualItems()->value<SimpleMissionItem*>(2);
    QVERIFY(item1);
    QVERIFY(item2);

    const double oldHomeAlt = settingsItem->editableAlt();
    const double oldAlt1 = item1->editableAlt();
    const double oldAlt2 = item2->editableAlt();

    const QGeoCoordinate expectedWp1 =
        home.atDistanceAndAzimuth(home.distanceTo(wp1), home.azimuthTo(wp1) + 90.0);
    const QGeoCoordinate expectedWp2 =
        home.atDistanceAndAzimuth(home.distanceTo(wp2), home.azimuthTo(wp2) + 90.0);

    _missionController->rotateMission(90.0, true, true);
    QVERIFY_TRUE_WAIT((settingsItem->coordinate().distanceTo(home) <= kCoordToleranceMeters) &&
                          (item1->coordinate().distanceTo(expectedWp1) <= kCoordToleranceMeters) &&
                          (item2->coordinate().distanceTo(expectedWp2) <= kCoordToleranceMeters),
                      TestTimeout::shortMs());

    QCOMPARE_COORDS(settingsItem->coordinate(), home, kCoordToleranceMeters);
    QCOMPARE_COORDS(item1->coordinate(), expectedWp1, kCoordToleranceMeters);
    QCOMPARE_COORDS(item2->coordinate(), expectedWp2, kCoordToleranceMeters);
    QCOMPARE_FUZZY(settingsItem->editableAlt(), oldHomeAlt, kAltToleranceMeters);
    QCOMPARE_FUZZY(item1->editableAlt(), oldAlt1, kAltToleranceMeters);
    QCOMPARE_FUZZY(item2->editableAlt(), oldAlt2, kAltToleranceMeters);
}

void MissionControllerTest::_testMissionTransformsInvalidHome()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    MissionSettingsItem* settingsItem =
        _missionController->visualItems()->value<MissionSettingsItem*>(0);
    QVERIFY(settingsItem);

    const QGeoCoordinate home = Coord::zurich();
    settingsItem->setCoordinate(home);

    const QGeoCoordinate wp1 = home.atDistanceAndAzimuth(180.0, 45.0);
    _missionController->insertSimpleMissionItem(wp1, 1);

    SimpleMissionItem* item1 = _missionController->visualItems()->value<SimpleMissionItem*>(1);
    QVERIFY(item1);

    const QGeoCoordinate oldItemCoord = item1->coordinate();
    const double oldItemAlt = item1->editableAlt();

    settingsItem->setCoordinate(QGeoCoordinate());
    QVERIFY_TRUE_WAIT(!settingsItem->coordinate().isValid(), TestTimeout::shortMs());

    // repositionMission and rotateMission require a valid home — they should be no-ops
    expectLogMessage("PlanManager.MissionController", QtWarningMsg, QRegularExpression("Cannot reposition mission while home is invalid"));
    _missionController->repositionMission(home.atDistanceAndAzimuth(100.0, 0.0), true, true);
    verifyExpectedLogMessage();
    expectLogMessage("PlanManager.MissionController", QtWarningMsg, QRegularExpression("Cannot rotate mission while home is invalid"));
    _missionController->rotateMission(45.0, true, true);
    verifyExpectedLogMessage();
    QCOMPARE_COORDS(item1->coordinate(), oldItemCoord, kCoordToleranceMeters);
    QCOMPARE_FUZZY(item1->editableAlt(), oldItemAlt, kAltToleranceMeters);

    // offsetMission does not depend on home — it should still apply
    _missionController->offsetMission(10.0, 5.0, 8.0, true, true);
    QVERIFY(item1->coordinate().distanceTo(oldItemCoord) > kCoordToleranceMeters);
    QCOMPARE_FUZZY(item1->editableAlt(), oldItemAlt + 8.0, kAltToleranceMeters);
}

void MissionControllerTest::_testLoadJsonSectionAvailable()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _masterController->loadFromFile(":/unittest/SectionTest.plan");
    QmlObjectListModel* visualItems = _missionController->visualItems();
    QVERIFY(visualItems);
    QCOMPARE(visualItems->count(), 5);
    // Check that only waypoint items have camera and speed sections
    for (int i = 1; i < visualItems->count(); i++) {
        SimpleMissionItem* item = visualItems->value<SimpleMissionItem*>(i);
        QVERIFY(item);
        if ((int)item->command() == MAV_CMD_NAV_WAYPOINT) {
            QCOMPARE(item->cameraSection()->available(), true);
            QCOMPARE(item->speedSection()->available(), true);
        } else {
            QCOMPARE(item->cameraSection()->available(), false);
            QCOMPARE(item->speedSection()->available(), false);
        }
    }
}

void MissionControllerTest::_testGlobalAltFrame()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    struct _globalAltFrame_s
    {
        QGroundControlQmlGlobal::AltitudeFrame altFrame;
        MAV_FRAME expectedMavFrame;
    } altFrameTestCases[] = {
        {QGroundControlQmlGlobal::AltitudeFrameRelative, MAV_FRAME_GLOBAL_RELATIVE_ALT},
        {QGroundControlQmlGlobal::AltitudeFrameAbsolute, MAV_FRAME_GLOBAL},
        {QGroundControlQmlGlobal::AltitudeFrameCalcAboveTerrain, MAV_FRAME_GLOBAL},
        {QGroundControlQmlGlobal::AltitudeFrameTerrain, MAV_FRAME_GLOBAL_TERRAIN_ALT},
    };

    for (const _globalAltFrame_s& testCase : altFrameTestCases) {
        _missionController->removeAll();
        _missionController->setGlobalAltitudeFrame(testCase.altFrame);
        // Use coordinate fixtures
        const QGeoCoordinate start = Coord::zurich();
        _missionController->insertTakeoffItem(start, 1);
        _missionController->insertSimpleMissionItem(start.atDistanceAndAzimuth(100, 0), 2);
        _missionController->insertSimpleMissionItem(start.atDistanceAndAzimuth(200, 0), 3);
        _missionController->insertSimpleMissionItem(start.atDistanceAndAzimuth(300, 0), 4);
        SimpleMissionItem* si =
            qobject_cast<SimpleMissionItem*>(_missionController->visualItems()->value<VisualMissionItem*>(1));
        QCOMPARE(si->altitudeFrame(), QGroundControlQmlGlobal::AltitudeFrameRelative);
        QCOMPARE(si->missionItem().frame(), MAV_FRAME_GLOBAL_RELATIVE_ALT);
        for (int i = 2; i < _missionController->visualItems()->count(); i++) {
            TEST_DEBUG(QStringLiteral("Validating altitude frame index %1").arg(i));
            SimpleMissionItem* siLoop =
                qobject_cast<SimpleMissionItem*>(_missionController->visualItems()->value<VisualMissionItem*>(i));
            QCOMPARE(siLoop->altitudeFrame(), testCase.altFrame);
            QCOMPARE(siLoop->missionItem().frame(), testCase.expectedMavFrame);
        }
    }
}

void MissionControllerTest::_testInsertComplexItemFromKML()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    // PolygonAreaTest.kml contains a 4-vertex polygon (5th coordinate is ring closure).
    // polyline.kml contains a LineString; CorridorScan expects a polyline, not a polygon.
    constexpr int kExpectedPolygonVertexCount = 4;
    const QString polygonKml = QStringLiteral(":/unittest/PolygonAreaTest.kml");
    const QString polylineKml = QStringLiteral(":/unittest/polyline.kml");

    // Survey — polygon KML defines the survey area
    VisualMissionItem* surveyVisual = _missionController->insertComplexMissionItemFromKMLOrSHP(
        SurveyComplexItem::canonicalName, polygonKml, 1, false);
    QVERIFY(surveyVisual);
    SurveyComplexItem* surveyItem = qobject_cast<SurveyComplexItem*>(surveyVisual);
    QVERIFY(surveyItem);
    QCOMPARE(surveyItem->surveyAreaPolygon()->count(), kExpectedPolygonVertexCount);

    // CorridorScan — polyline KML defines the corridor path; corridor polygon is computed from it
    VisualMissionItem* corridorVisual = _missionController->insertComplexMissionItemFromKMLOrSHP(
        CorridorScanComplexItem::canonicalName, polylineKml, 2, false);
    QVERIFY(corridorVisual);
    CorridorScanComplexItem* corridorItem = qobject_cast<CorridorScanComplexItem*>(corridorVisual);
    QVERIFY(corridorItem);
    QVERIFY(corridorItem->surveyAreaPolygon()->count() > 0);

    // StructureScan — polygon KML defines the structure perimeter
    VisualMissionItem* structureVisual = _missionController->insertComplexMissionItemFromKMLOrSHP(
        StructureScanComplexItem::canonicalName, polygonKml, 3, false);
    QVERIFY(structureVisual);
    StructureScanComplexItem* structureItem = qobject_cast<StructureScanComplexItem*>(structureVisual);
    QVERIFY(structureItem);
    QCOMPARE(structureItem->structurePolygon()->count(), kExpectedPolygonVertexCount);

    // home + Survey + CorridorScan + StructureScan
    QCOMPARE(_missionController->visualItems()->count(), 4);
}

void MissionControllerTest::_testLoadPlanRoundTripComplexItems()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    const QGeoCoordinate center = Coord::zurich();

    // Insert a Survey and a CorridorScan
    VisualMissionItem* surveyVisual = _missionController->insertComplexMissionItem(
        SurveyComplexItem::canonicalName, center, 1, false);
    QVERIFY(surveyVisual);
    QVERIFY(qobject_cast<SurveyComplexItem*>(surveyVisual));

    VisualMissionItem* corridorVisual = _missionController->insertComplexMissionItem(
        CorridorScanComplexItem::canonicalName, center, 2, false);
    QVERIFY(corridorVisual);
    QVERIFY(qobject_cast<CorridorScanComplexItem*>(corridorVisual));

    // home + Survey + CorridorScan
    QCOMPARE(_missionController->visualItems()->count(), 3);

    // Save to a temporary file, reload, and verify the types survive the round-trip
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString planPath = QStringLiteral("%1/test.%2").arg(tmpDir.path(), _masterController->fileExtension());
    QVERIFY(_masterController->saveToFile(planPath));

    _missionController->removeAll();
    QCOMPARE(_missionController->visualItems()->count(), 1); // home only

    _masterController->loadFromFile(planPath);
    QCOMPARE(_missionController->visualItems()->count(), 3);

    QVERIFY(qobject_cast<SurveyComplexItem*>(_missionController->visualItems()->value<VisualMissionItem*>(1)));
    QVERIFY(qobject_cast<CorridorScanComplexItem*>(_missionController->visualItems()->value<VisualMissionItem*>(2)));
}

void MissionControllerTest::_testInsertSurveyAppliesAltFrameInMixedMode()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _missionController->setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameMixed);

    const QGeoCoordinate coord = Coord::zurich();

    // Insert a simple waypoint and force its altitude frame to Absolute
    VisualMissionItem* simpleVisual = _missionController->insertSimpleMissionItem(coord, 1, false);
    QVERIFY(simpleVisual);
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(simpleVisual);
    QVERIFY(simpleItem);
    simpleItem->setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameAbsolute);
    simpleItem->altitude()->setRawValue(150.0);

    // Survey default distanceMode is Relative; inserting after an Absolute item should change it
    QCOMPARE(_missionController->visualItems()->count(), 2); // home + simple
    VisualMissionItem* surveyVisual = _missionController->insertComplexMissionItem(
        SurveyComplexItem::canonicalName, coord, 2, false);
    QVERIFY(surveyVisual);
    SurveyComplexItem* surveyItem = qobject_cast<SurveyComplexItem*>(surveyVisual);
    QVERIFY(surveyItem);

    QCOMPARE(surveyItem->cameraCalc()->distanceMode(), QGroundControlQmlGlobal::AltitudeFrameAbsolute);
}

void MissionControllerTest::_testInsertNonSurveyComplexItemMixedModeNoCrash()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);
    _missionController->setGlobalAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameMixed);

    const QGeoCoordinate coord = Coord::zurich();

    // Insert a simple waypoint with Absolute frame to provide altitude context
    VisualMissionItem* simpleVisual = _missionController->insertSimpleMissionItem(coord, 1, false);
    QVERIFY(simpleVisual);
    SimpleMissionItem* simpleItem = qobject_cast<SimpleMissionItem*>(simpleVisual);
    QVERIFY(simpleItem);
    simpleItem->setAltitudeFrame(QGroundControlQmlGlobal::AltitudeFrameAbsolute);

    // CorridorScan inherits the base-class no-op — distanceMode stays at its default (Relative)
    VisualMissionItem* corridorVisual = _missionController->insertComplexMissionItem(
        CorridorScanComplexItem::canonicalName, coord, 2, false);
    QVERIFY(corridorVisual);
    CorridorScanComplexItem* corridorItem = qobject_cast<CorridorScanComplexItem*>(corridorVisual);
    QVERIFY(corridorItem);

    QCOMPARE(corridorItem->cameraCalc()->distanceMode(), QGroundControlQmlGlobal::AltitudeFrameRelative);
    QCOMPARE(_missionController->visualItems()->count(), 3);
}

#include "UnitTest.h"

UT_REGISTER_TEST(MissionControllerTest, TestLabel::Integration, TestLabel::MissionManager)
