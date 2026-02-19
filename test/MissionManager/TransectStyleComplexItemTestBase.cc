#include "TransectStyleComplexItemTestBase.h"

#include "MissionItem.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"
#include "SettingsManager.h"

void TransectStyleComplexItemTestBase::init()
{
    OfflineMissionTest::init();

    _planViewSettings = SettingsManager::instance()->planViewSettings();
    _controllerVehicle = planController()->controllerVehicle();
}

void TransectStyleComplexItemTestBase::cleanup()
{
    _planViewSettings = nullptr;
    _controllerVehicle = nullptr;

    OfflineMissionTest::cleanup();
}

void TransectStyleComplexItemTestBase::_printItemCommands(QList<MissionItem*> items)
{
    // Handy for debugging failures
    for (int i = 0; i < items.count(); i++) {
        MissionItem* item = items[i];
        qDebug() << "Index:Cmd" << i << item->command();
    }
}
