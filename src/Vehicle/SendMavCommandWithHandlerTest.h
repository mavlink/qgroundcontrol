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
#include "Vehicle.h"

class SendMavCommandWithHandlerTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void _performTestCases(void);
    void _compIdAllFailure(void);
    void _duplicateCommand(void);

private:
    typedef struct {
        MAV_CMD                             command;
        MAV_RESULT                          expectedCommandResult;
        uint8_t                             progress;
        Vehicle::MavCmdResultFailureCode_t  expectedFailureCode;
        int                                 expectedSendCount;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static void _mavCmdResultHandler            (void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress, Vehicle::MavCmdResultFailureCode_t failureCode);
    static void _compIdAllMavCmdResultHandler   (void* resultHandlerData, int compId, MAV_RESULT commandResult, uint8_t progress, Vehicle::MavCmdResultFailureCode_t failureCode);

    static bool _handlerCalled;

    static TestCase_t _rgTestCases[];
};
