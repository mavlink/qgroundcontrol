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
        bool                                expectInProgressResult;
        Vehicle::MavCmdResultFailureCode_t  expectedFailureCode;
        int                                 expectedSendCount;
    } TestCase_t;

    void _testCaseWorker(TestCase_t& testCase);

    static void _mavCmdResultHandler                (void* resultHandlerData,   int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);
    static void _mavCmdProgressHandler              (void* progressHandlerData, int compId, const mavlink_command_ack_t& ack);
    static void _compIdAllFailureMavCmdResultHandler(void* resultHandlerData,   int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode);

    static bool _resultHandlerCalled;
    static bool _progressHandlerCalled;

    static TestCase_t _rgTestCases[];
};
