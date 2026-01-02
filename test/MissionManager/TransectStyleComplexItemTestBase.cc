#include "TransectStyleComplexItemTestBase.h"
#include "SettingsManager.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"
#include "MissionItem.h"

#include <QtTest/QTest>

TransectStyleComplexItemTestBase::TransectStyleComplexItemTestBase(void)
{
}

void TransectStyleComplexItemTestBase::init(void)
{
    UnitTest::init();

    _planViewSettings = SettingsManager::instance()->planViewSettings();
    _masterController = new PlanMasterController(this);
    _controllerVehicle = _masterController->controllerVehicle();
}

void TransectStyleComplexItemTestBase::cleanup(void)
{
    delete _masterController;

    _planViewSettings   = nullptr;
    _masterController   = nullptr;
    _controllerVehicle  = nullptr;

    UnitTest::cleanup();
}

void TransectStyleComplexItemTestBase::_printItemCommands(QList<MissionItem*> items)
{
    // Handy for debugging failures
    for (int i=0; i<items.count(); i++) {
        MissionItem* item = items[i];
        qDebug() << "Index:Cmd" << i << item->command();
    }
}
