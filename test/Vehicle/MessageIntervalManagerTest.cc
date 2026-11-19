#include "MessageIntervalManagerTest.h"

#include "Fixtures/RAIIFixtures.h"
#include "MavCommandQueue.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

void MessageIntervalManagerTest::_setMessageRate(Vehicle* vehicle, uint16_t msgId, int32_t rate)
{
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, msgId, rate);
    QVERIFY_TRUE_WAIT(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL),
                      TestTimeout::longMs());
}

void MessageIntervalManagerTest::_waitIdle(Vehicle* vehicle)
{
    QVERIFY_TRUE_WAIT(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL),
                      TestTimeout::longMs());
    _mockLink->clearReceivedMavCommandCounts();
}

void MessageIntervalManagerTest::_refusedRateNotRequestedAgain()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    _waitIdle(vehicle);

    // MockLink refuses SET_MESSAGE_INTERVAL for messages it doesn't stream
    _setMessageRate(vehicle, MAVLINK_MSG_ID_DEBUG);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);

    // ...so it isn't asked again
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG, 1);
    QVERIFY(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL));
    QTest::qWait(TestTimeout::shortMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 1);
}

void MessageIntervalManagerTest::_refusedRateOtherRateRequested()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    _waitIdle(vehicle);

    // MAV_RESULT_DENIED can refuse just the requested interval, so a different rate is still sent
    _setMessageRate(vehicle, MAVLINK_MSG_ID_DEBUG, 1);
    _setMessageRate(vehicle, MAVLINK_MSG_ID_DEBUG, 2);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2);
}

void MessageIntervalManagerTest::_acceptedRateRequestedAgain()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    _waitIdle(vehicle);

    // MockLink accepts SET_MESSAGE_INTERVAL for VFR_HUD; repeating the request still sends it, as
    // a vehicle reboot loses the rate
    _setMessageRate(vehicle, MAVLINK_MSG_ID_VFR_HUD);
    _setMessageRate(vehicle, MAVLINK_MSG_ID_VFR_HUD);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2);
}

void MessageIntervalManagerTest::_acceptedRatesConfirmedOneAtATime()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    _waitIdle(vehicle);

    // MockLink doesn't answer MESSAGE_INTERVAL requests, so the first rate's confirmation is still
    // outstanding when the second rate is accepted. The second confirmation is sent after the first
    // gives up, rather than failing as a duplicate.
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout;
    ignoreLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries: MAV_CMD_REQUEST_MESSAGE "
                                        "message: MESSAGE_INTERVAL"));
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_MESSAGE_INTERVAL);
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_VFR_HUD, 1);
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_ATTITUDE_TARGET, 1);
    QVERIFY_TRUE_WAIT(_mockLink->receivedRequestMessageCount(MAVLINK_MSG_ID_MESSAGE_INTERVAL) ==
                              2 * MavCommandQueue::kMaxRetryCount &&
                          !vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_REQUEST_MESSAGE),
                      TestTimeout::longMs());
    _mockLink->setRequestMessageNoResponse(MAVLINK_MSG_ID_MESSAGE_INTERVAL, false);
}

void MessageIntervalManagerTest::_overlappingRefusalsRemembered()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    _waitIdle(vehicle);

    // Requests made together are sent one at a time, so each refusal can be attributed
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG, 1);
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG_VECT, 1);
    _waitIdle(vehicle);

    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG, 1);
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_DEBUG_VECT, 1);
    QTest::qWait(TestTimeout::shortMs());
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 0);
}

void MessageIntervalManagerTest::_refusalAfterLostAckNotRemembered()
{
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    QVERIFY(vehicle);
    _waitIdle(vehicle);

    TestFixtures::MavCommandAckTimeoutFixture ackTimeout;
    _mockLink->setSetMessageIntervalNoAck(MAVLINK_MSG_ID_VFR_HUD);

    // VFR_HUD's ack is lost, so the refusal of a SET_MESSAGE_INTERVAL sent directly afterwards is
    // matched to the VFR_HUD request. It mustn't be remembered as a refusal of VFR_HUD.
    expectLogMessage("Vehicle.MavCommandQueue", QtWarningMsg,
                     QRegularExpression("Giving up sending command after max retries: MAV_CMD_SET_MESSAGE_INTERVAL "
                                        "message: DEBUG"));
    vehicle->setMessageRate(MAV_COMP_ID_AUTOPILOT1, MAVLINK_MSG_ID_VFR_HUD, 1);
    vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL, false /* showError */,
                            MAVLINK_MSG_ID_DEBUG, 1000000);
    QVERIFY_TRUE_WAIT(!vehicle->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_SET_MESSAGE_INTERVAL),
                      TestTimeout::longMs());
    verifyExpectedLogMessage();
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 2);

    _mockLink->setSetMessageIntervalNoAck(MAVLINK_MSG_ID_VFR_HUD, false);
    _setMessageRate(vehicle, MAVLINK_MSG_ID_VFR_HUD);
    QCOMPARE(_mockLink->receivedMavCommandCount(MAV_CMD_SET_MESSAGE_INTERVAL), 3);
}

UT_REGISTER_TEST(MessageIntervalManagerTest, TestLabel::Integration, TestLabel::Vehicle)
