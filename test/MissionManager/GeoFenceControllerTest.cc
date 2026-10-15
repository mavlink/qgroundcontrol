#include "GeoFenceControllerTest.h"

#include <QtCore/QJsonObject>
#include <QtTest/QSignalSpy>

#include "CoordFixtures.h"
#include "GeoFenceController.h"
#include "GeoFenceManager.h"
#include "MissionController.h"
#include "MissionItem.h"
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
    GeoFenceController* const controller = geoFenceController();
    missionController()->setHomePosition(TestFixtures::Coord::zurich());

    QGCFenceCircle* const homeCircle = addCircle(controller);
    homeCircle->setHomeCentered(true);
    QGCFenceCircle* const fixedCircle = addCircle(controller);
    const QGeoCoordinate fixedCenter = fixedCircle->center();
    controller->setDirty(false);

    const QGeoCoordinate newHome = TestFixtures::Coord::zurich().atDistanceAndAzimuth(500, 0);
    missionController()->setHomePosition(newHome);

    QCOMPARE_COORDS(homeCircle->center(), newHome);
    QCOMPARE(fixedCircle->center(), fixedCenter);
    QVERIFY(!controller->dirty());
}

void GeoFenceControllerTest::_testHomeCircleJsonRoundTrip()
{
    GeoFenceController* const controller = geoFenceController();
    missionController()->setHomePosition(TestFixtures::Coord::zurich());

    addCircle(controller)->setHomeCentered(true);
    addCircle(controller);

    QJsonObject json;
    controller->save(json);

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
