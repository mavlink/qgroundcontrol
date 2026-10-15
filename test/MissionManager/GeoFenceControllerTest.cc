#include "GeoFenceControllerTest.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "CoordFixtures.h"
#include "GeoFenceController.h"
#include "GeoFenceManager.h"
#include "JsonParsing.h"
#include "MissionController.h"
#include "MissionItem.h"
#include "PlanMasterController.h"
#include "QGCFenceCircle.h"
#include "QmlObjectListModel.h"
#include "UnitTestCoords.h"
#include "Vehicle.h"

namespace {

QGeoCoordinate home2D()
{
    const QGeoCoordinate home = TestFixtures::Coord::zurich();
    return QGeoCoordinate(home.latitude(), home.longitude());
}

/// Adds a circle away from home and returns it
QGCFenceCircle* addCircle(GeoFenceController* controller)
{
    const QGeoCoordinate center = home2D().atDistanceAndAzimuth(1000, 90);
    controller->addInclusionCircle(center.atDistanceAndAzimuth(300, 315), center.atDistanceAndAzimuth(300, 135));
    return controller->circles()->value<QGCFenceCircle*>(controller->circles()->count() - 1);
}

}  // namespace

GeoFenceControllerTest::GeoFenceControllerTest(QObject* parent)
    : MissionTest(parent)
{
    setAutopilotType(MAV_AUTOPILOT_ARDUPILOTMEGA);
}

void GeoFenceControllerTest::_testHomeCircleSupportedVersion_data()
{
    QTest::addColumn<int>("major");
    QTest::addColumn<int>("minor");
    QTest::addColumn<int>("patch");
    QTest::addColumn<bool>("supported");

    QTest::newRow("4.7.9") << 4 << 7 << 9 << false;
    QTest::newRow("4.8.0") << 4 << 8 << 0 << true;
    QTest::newRow("5.0.0") << 5 << 0 << 0 << true;
}

void GeoFenceControllerTest::_testHomeCircleSupportedVersion()
{
    QFETCH(int, major);
    QFETCH(int, minor);
    QFETCH(int, patch);
    QFETCH(bool, supported);

    QSignalSpy spy(geoFenceController(), &GeoFenceController::homeCircleSupportedChanged);
    vehicle()->setFirmwareVersion(major, minor, patch);
    QVERIFY(spy.count() >= 1);
    QCOMPARE(geoFenceController()->homeCircleSupported(), supported);
}

void GeoFenceControllerTest::_testHomeCircleVehicleRoundTrip()
{
    GeoFenceController* const controller = geoFenceController();
    missionController()->setHomePosition(TestFixtures::Coord::zurich());

    QGCFenceCircle* circle = addCircle(controller);
    QVERIFY(circle);
    circle->setInclusion(false);
    circle->radius()->setRawValue(75.0);
    circle->setHomeCentered(true);
    QVERIFY(circle->inclusion());
    QCOMPARE_COORDS(circle->center(), home2D());

    GeoFenceManager* const manager = vehicle()->geoFenceManager();
    QSignalSpy sendSpy(manager, &GeoFenceManager::sendComplete);
    controller->sendToVehicle();
    QTRY_COMPARE_WITH_TIMEOUT(sendSpy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(sendSpy.takeFirst().at(0).toBool(), false);

    QSignalSpy loadSpy(controller, &GeoFenceController::loadComplete);
    controller->loadFromVehicle();
    QTRY_COMPARE_WITH_TIMEOUT(loadSpy.count(), 1, TestTimeout::mediumMs());

    QCOMPARE(manager->missionItems().count(), 1);
    const MissionItem* item = manager->missionItems().first();
    QCOMPARE(item->command(), MAV_CMD_NAV_FENCE_HOME_CIRCLE_INCLUSION);
    QCOMPARE(item->param1(), 75.0);
    QCOMPARE(item->param5(), 0.0);
    QCOMPARE(item->param6(), 0.0);

    QCOMPARE(controller->circles()->count(), 1);
    circle = controller->circles()->value<QGCFenceCircle*>(0);
    QVERIFY(circle->homeCentered());
    QVERIFY(circle->inclusion());
    QCOMPARE(circle->radius()->rawValue().toDouble(), 75.0);
    QCOMPARE_COORDS(circle->center(), home2D());
    QVERIFY(!controller->dirty());
}

void GeoFenceControllerTest::_testHomeCircleFollowsHomeWithoutDirty()
{
    vehicle()->setFirmwareVersion(4, 8, 0);
    missionController()->setHomePosition(TestFixtures::Coord::zurich());
    GeoFenceController* const controller = geoFenceController();
    addCircle(controller)->setHomeCentered(true);
    addCircle(controller);

    // Start from a plan that is clean for both upload and save
    planController()->sendToVehicle();
    QTRY_VERIFY_WITH_TIMEOUT(!planController()->syncInProgress() && !planController()->dirtyForUpload(),
                             TestTimeout::mediumMs());
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(planController()->saveToFile(tempDir.filePath(QStringLiteral("home.plan"))));
    QVERIFY(!planController()->dirtyForSave());

    // Fly view reloaded the fence from the vehicle after the upload
    QCOMPARE(controller->circles()->count(), 2);
    QGCFenceCircle* const homeCircle = controller->circles()->value<QGCFenceCircle*>(0);
    QGCFenceCircle* const fixedCircle = controller->circles()->value<QGCFenceCircle*>(1);
    QVERIFY(homeCircle->homeCentered());
    const QGeoCoordinate fixedCenter = fixedCircle->center();

    // PlanMasterController latches the plan dirty on any of these, even if later cleared
    QSignalSpy dirtySpy(controller, &GeoFenceController::dirtyChanged);
    const QGeoCoordinate newHome = TestFixtures::Coord::zurich().atDistanceAndAzimuth(500, 0);
    Vehicle* const flyVehicle = vehicle();
    emit flyVehicle->homePositionChanged(newHome);

    QCOMPARE_COORDS(homeCircle->center(), newHome);
    QCOMPARE(fixedCircle->center(), fixedCenter);
    QCOMPARE(dirtySpy.count(), 0);
    QVERIFY(!controller->dirty());
    QVERIFY(!planController()->dirtyForSave());
    QVERIFY(!planController()->dirtyForUpload());
}

void GeoFenceControllerTest::_testUploadRefusedWhenHomeCircleUnsupported()
{
    vehicle()->setFirmwareVersion(4, 7, 0);
    GeoFenceController* const controller = geoFenceController();
    QVERIFY(!controller->homeCircleSupported());

    QSignalSpy hasHomeSpy(controller, &GeoFenceController::hasHomeCircleChanged);
    QGCFenceCircle* const circle = addCircle(controller);
    QVERIFY(!controller->hasHomeCircle());
    circle->setHomeCentered(true);
    QVERIFY(controller->hasHomeCircle());
    QVERIFY(hasHomeSpy.count() >= 2);

    expectAppMessage(QRegularExpression(QStringLiteral("does not support home-centered")));
    planController()->sendToVehicle();
    verifyExpectedLogMessage();
    QVERIFY(!planController()->syncInProgress());
    QVERIFY(vehicle()->geoFenceManager()->missionItems().isEmpty());

    circle->setHomeCentered(false);
    QVERIFY(!controller->hasHomeCircle());
}

void GeoFenceControllerTest::_testHomeCircleJsonRoundTrip()
{
    GeoFenceController* const controller = geoFenceController();
    missionController()->setHomePosition(TestFixtures::Coord::zurich());

    addCircle(controller)->setHomeCentered(true);
    addCircle(controller);

    QJsonObject json;
    controller->save(json);

    // Home circles use a newer version so older QGC rejects them; ordinary circles stay readable
    const QJsonArray jsonCircles = json[QStringLiteral("circles")].toArray();
    QCOMPARE(jsonCircles.count(), 2);
    QCOMPARE(jsonCircles[0].toObject()[JsonParsing::jsonVersionKey].toInt(), 2);
    QCOMPARE(jsonCircles[1].toObject()[JsonParsing::jsonVersionKey].toInt(), 1);
    QVERIFY(!jsonCircles[1].toObject().contains(QStringLiteral("homeCentered")));

    const QGeoCoordinate newHome = TestFixtures::Coord::zurich().atDistanceAndAzimuth(500, 0);
    missionController()->setHomePosition(newHome);

    QString errorString;
    QVERIFY2(controller->load(json, errorString), qPrintable(errorString));
    QCOMPARE(controller->circles()->count(), 2);

    const QGCFenceCircle* const homeCircle = controller->circles()->value<QGCFenceCircle*>(0);
    QVERIFY(homeCircle->homeCentered());
    QCOMPARE_COORDS(homeCircle->center(), newHome);
    QVERIFY(!controller->circles()->value<QGCFenceCircle*>(1)->homeCentered());
    QVERIFY(!controller->dirty());
}

UT_REGISTER_TEST(GeoFenceControllerTest, TestLabel::Integration, TestLabel::MissionManager)
