#include "MissionTest.h"

#include <QtTest/QTest>

#include "AppSettings.h"
#include "GeoFenceController.h"
#include "MissionController.h"
#include "PlanMasterController.h"
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
