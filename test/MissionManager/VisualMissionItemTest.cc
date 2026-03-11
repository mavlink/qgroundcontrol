#include "VisualMissionItemTest.h"

#include "MultiSignalSpy.h"
#include "PlanMasterController.h"
#include "VisualMissionItem.h"

void VisualMissionItemTest::init()
{
    OfflineMissionTest::init();
    _controllerVehicle = planController()->controllerVehicle();
}

void VisualMissionItemTest::_createSpy(VisualMissionItem* visualItem, MultiSignalSpy** visualSpy)
{
    *visualSpy = nullptr;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QVERIFY(spy->init(visualItem));
    *visualSpy = spy;
}
