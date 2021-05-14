/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InitialConnectTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "MockLink.h"

void InitialConnectTest::_performTestCases(void)
{
    static const struct TestCase_s {
        MockConfiguration::FailureMode_t    failureMode;
        const char*                         failureModeStr;
    } rgTestCases[] = {
    { MockConfiguration::FailNone,                                                   "No failures" },
    { MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionFailure,    "REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure" },
    { MockConfiguration::FailInitialConnectRequestMessageAutopilotVersionLost,       "REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent" },
    { MockConfiguration::FailInitialConnectRequestMessageProtocolVersionFailure,     "REQUEST_MESSAGE:PROTOCOL_VERSION returns failure" },
    { MockConfiguration::FailInitialConnectRequestMessageProtocolVersionLost,        "REQUEST_MESSAGE:PROTOCOL_VERSION success, PROTOCOL_VERSION never sent" },
    };

    for (const struct TestCase_s& testCase: rgTestCases) {
        qDebug() << "Testing case failure mode:" << testCase.failureModeStr;
        _connectMockLink(MAV_AUTOPILOT_PX4, testCase.failureMode);
        _disconnectMockLink();
    }
}
