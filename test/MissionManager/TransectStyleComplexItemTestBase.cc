/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
