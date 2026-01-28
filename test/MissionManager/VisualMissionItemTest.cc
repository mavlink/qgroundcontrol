#include "VisualMissionItemTest.h"
#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "VisualMissionItem.h"

#include <QtTest/QTest>

void VisualMissionItemTest::init()
{
    OfflineTest::init();

    _masterController = new PlanMasterController(MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR, this);
    VERIFY_NOT_NULL(_masterController);
    _controllerVehicle = _masterController->controllerVehicle();
    VERIFY_NOT_NULL(_controllerVehicle);
}

void VisualMissionItemTest::cleanup()
{
    delete _masterController;

    _masterController   = nullptr;
    _controllerVehicle  = nullptr;

    OfflineTest::cleanup();
}

void VisualMissionItemTest::_createSpy(VisualMissionItem* visualItem, MultiSignalSpy** visualSpy)
{
    *visualSpy = nullptr;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QVERIFY(spy->init(visualItem));
    *visualSpy = spy;
}
