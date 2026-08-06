#include "MavCommandQueueReentrancyTest.h"

#include <QtCore/QRegularExpression>

#include "MultiVehicleManager.h"

void MavCommandQueueReentrancyTest::_closeVehicleResultHandler(void* resultHandlerData, int compId,
                                                               const mavlink_command_ack_t& ack,
                                                               Vehicle::MavCmdResultFailureCode_t failureCode)
{
    auto* self = static_cast<MavCommandQueueReentrancyTest*>(resultHandlerData);
    self->_resultHandlerCallCount++;

    QCOMPARE(compId, MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE(ack.result, MAV_RESULT_FAILED);
    QCOMPARE(failureCode, Vehicle::MavCmdResultFailureNoResponseToCommand);

    // Simulate the user closing log replay / disconnecting the vehicle from within the
    // give-up callback chain. This synchronously calls Vehicle::_stopCommandProcessing()
    // which clears the pending command list while _responseTimeoutCheck is iterating it.
    self->vehicle()->closeVehicle();
}

void MavCommandQueueReentrancyTest::_testCloseVehicleFromGiveUpHandler()
{
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries:"));

    _resultHandlerCallCount = 0;

    // First entry (index 0): retried command which stays pending in the list across
    // multiple timeout checks.
    vehicle()->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, false /* showError */);

    // Second entry (index 1): no-retry command which gives up on the first ack timeout.
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler = _closeVehicleResultHandler;
    handlerInfo.resultHandlerData = this;
    vehicle()->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1,
                                         MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE_NO_RETRY);

    // _responseTimeoutCheck walks the list backwards. The no-retry command (index 1) gives
    // up first; its result handler closes the vehicle which clears the pending list. The
    // loop must then not touch index 0 of the now-empty list (issue #13251 crash 3 -
    // out-of-bounds access in a Debug build aborts here).
    QTRY_COMPARE_WITH_TIMEOUT(_resultHandlerCallCount, 1, TestTimeout::longMs());

    // closeVehicle() tears the vehicle down asynchronously; wait for it to complete so
    // any deferred crash in the timeout-check loop has a chance to fire before cleanup.
    QTRY_VERIFY_WITH_TIMEOUT(MultiVehicleManager::instance()->activeVehicle() == nullptr, TestTimeout::longMs());
}

UT_REGISTER_TEST(MavCommandQueueReentrancyTest, TestLabel::Integration, TestLabel::Vehicle)
