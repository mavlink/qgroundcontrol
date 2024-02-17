/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "CorridorScanComplexItem.h"
#include "PlanMasterController.h"
#include "PlanViewSettings.h"

/// Base class for all TransectStyleComplexItem unit tests
class TransectStyleComplexItemTestBase : public UnitTest
{
    Q_OBJECT
    
public:
    TransectStyleComplexItemTestBase(void);

protected:
    void init   (void) override;
    void cleanup(void) override;

    void _printItemCommands(QList<MissionItem*> items);

    PlanMasterController*   _masterController =     nullptr;
    Vehicle*                _controllerVehicle =    nullptr;
    PlanViewSettings*       _planViewSettings =     nullptr;
};
