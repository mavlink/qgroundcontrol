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

class SendMavCommandWithHandlerTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void _performTestCases(void);

private:
    typedef struct {
        MAV_CMD     command;
        MAV_RESULT  expectedCommandResult;
        bool        expectedNoResponseFromVehicle;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static void _mavCmdResultHandler(void* resultHandlerData, int compId, MAV_RESULT commandResult, bool noResponsefromVehicle);

    static TestCase_t _rgTestCases[];
};
