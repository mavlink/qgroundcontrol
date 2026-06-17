#include "MissionTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

#include "AppSettings.h"
#include "Fact.h"
#include "GeoFenceController.h"
#include "MissionController.h"
#include "MissionItem.h"
#include "PlanMasterController.h"
#include "QGCMath.h"
#include "QGCMAVLink.h"
#include "RallyPointController.h"
#include "SettingsManager.h"

// ============================================================================
// MissionTest Implementation
// ============================================================================

MissionTest::MissionTest(QObject* parent) : VehicleTest(parent)
{
}

void MissionTest::init()
{
    VehicleTest::init();

    _planController = new PlanMasterController(this);
    _planController->setFlyView(_flyView);
    _planController->start();

    if (_vehicle) {
        _planController->startStaticActiveVehicle(_vehicle);
    }
}

void MissionTest::cleanup()
{
    delete _planController;
    _planController = nullptr;

    VehicleTest::cleanup();
}

MissionController* MissionTest::missionController() const
{
    return _planController ? _planController->missionController() : nullptr;
}

GeoFenceController* MissionTest::geoFenceController() const
{
    return _planController ? _planController->geoFenceController() : nullptr;
}

RallyPointController* MissionTest::rallyPointController() const
{
    return _planController ? _planController->rallyPointController() : nullptr;
}

void MissionTest::clearMission()
{
    if (missionController()) {
        missionController()->removeAll();
    }
}

void MissionTest::clearAllPlanData()
{
    clearMission();
    if (geoFenceController()) {
        geoFenceController()->removeAll();
    }
    if (rallyPointController()) {
        rallyPointController()->removeAll();
    }
}

// ============================================================================
// OfflineMissionTest Implementation
// ============================================================================

OfflineMissionTest::OfflineMissionTest(QObject* parent) : UnitTest(parent)
{
}

void OfflineMissionTest::init()
{
    UnitTest::init();

    // Configure offline editing settings
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(_offlineFirmwareType));
    appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(_offlineVehicleType));

    _planController = new PlanMasterController(this);
    _planController->setFlyView(false);
    _planController->start();
}

void OfflineMissionTest::cleanup()
{
    delete _planController;
    _planController = nullptr;

    UnitTest::cleanup();
}

MissionController* OfflineMissionTest::missionController() const
{
    return _planController ? _planController->missionController() : nullptr;
}

GeoFenceController* OfflineMissionTest::geoFenceController() const
{
    return _planController ? _planController->geoFenceController() : nullptr;
}

RallyPointController* OfflineMissionTest::rallyPointController() const
{
    return _planController ? _planController->rallyPointController() : nullptr;
}

void OfflineMissionTest::clearMission()
{
    if (missionController()) {
        missionController()->removeAll();
    }
}

void OfflineMissionTest::_missionItemsEqual(const MissionItem& actual, const MissionItem& expected)
{
    QCOMPARE(static_cast<int>(actual.command()), static_cast<int>(expected.command()));
    QCOMPARE(static_cast<int>(actual.frame()), static_cast<int>(expected.frame()));
    QCOMPARE(actual.autoContinue(), expected.autoContinue());

    QVERIFY(QGC::fuzzyCompare(actual.param1(), expected.param1()));
    QVERIFY(QGC::fuzzyCompare(actual.param2(), expected.param2()));
    QVERIFY(QGC::fuzzyCompare(actual.param3(), expected.param3()));
    QVERIFY(QGC::fuzzyCompare(actual.param4(), expected.param4()));
    QVERIFY(QGC::fuzzyCompare(actual.param5(), expected.param5()));
    QVERIFY(QGC::fuzzyCompare(actual.param6(), expected.param6()));
    QVERIFY(QGC::fuzzyCompare(actual.param7(), expected.param7()));
}

void OfflineMissionTest::changeFactValue(Fact* fact, double increment)
{
    if (fact->typeIsBool()) {
        fact->setRawValue(!fact->rawValue().toBool());
    } else {
        if (qFuzzyIsNull(increment)) {
            increment = 1.0;
        }
        fact->setRawValue(fact->rawValue().toDouble() + increment);
    }
}

QGeoCoordinate OfflineMissionTest::changeCoordinateValue(const QGeoCoordinate& coordinate)
{
    return coordinate.atDistanceAndAzimuth(1, 0);
}
