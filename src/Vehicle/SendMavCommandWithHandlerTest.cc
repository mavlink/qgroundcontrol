/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SendMavCommandWithHandlerTest.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "MockLink.h"

SendMavCommandWithHandlerTest::TestCase_t SendMavCommandWithHandlerTest::_rgTestCases[] = {
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,           MAV_RESULT_ACCEPTED, false },
    {  MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED,             MAV_RESULT_ACCEPTED, false },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_ACCEPTED,   MAV_RESULT_ACCEPTED, false },
    {  MockLink::MAV_CMD_MOCKLINK_SECOND_ATTEMPT_RESULT_FAILED,     MAV_RESULT_ACCEPTED, false },
    {  MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE,                      MAV_RESULT_ACCEPTED, false },
};

void SendMavCommandWithHandlerTest::_mavCmdResultHandler(void* resultHandlerData, int compId, MAV_RESULT commandResult, bool noResponsefromVehicle)
{
    TestCase_t* testCase = static_cast<TestCase_t*>(resultHandlerData);

    QVERIFY(compId == MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE(testCase->expectedCommandResult, commandResult);
    QCOMPARE(testCase->expectedNoResponseFromVehicle, noResponsefromVehicle);
}

void SendMavCommandWithHandlerTest::_testCaseWorker(TestCase_t& testCase)
{
    _connectMockLinkNoInitialConnectSequence();

    MultiVehicleManager*    vehicleMgr  = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle*                vehicle     = vehicleMgr->activeVehicle();

    vehicle->sendMavCommandWithHandler(_mavCmdResultHandler, &testCase, MAV_COMP_ID_ALL, testCase.command);

    _disconnectMockLink();
}

void SendMavCommandWithHandlerTest::_performTestCases(void)
{
    int index = 0;
    for (TestCase_t& testCase: _rgTestCases) {
        qDebug() << "Testing case" << index++;
        _testCaseWorker(testCase);
    }
}
