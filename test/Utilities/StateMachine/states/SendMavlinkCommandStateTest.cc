#include "SendMavlinkCommandStateTest.h"

#include <QtCore/QRegularExpression>

#include "Fixtures/RAIIFixtures.h"
#include "MockLink.h"
#include "SendMavlinkCommandState.h"
#include "StateTestCommon.h"
#include "Vehicle.h"

void SendMavlinkCommandStateTest::_testCommandResult_data()
{
    QTest::addColumn<MAV_CMD>("command");
    QTest::addColumn<bool>("setUpAfterConstruction");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("accepted") << MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED << false << true;
    QTest::newRow("accepted, set up after construction")
        << MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED << true << true;
    QTest::newRow("failed") << MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_FAILED << false << false;
}

void SendMavlinkCommandStateTest::_testCommandResult()
{
    QFETCH(MAV_CMD, command);
    QFETCH(bool, setUpAfterConstruction);
    QFETCH(bool, accepted);

    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    SendMavlinkCommandState* state = nullptr;
    if (setUpAfterConstruction) {
        state = new SendMavlinkCommandState(&machine);
        state->setup(command);
    } else {
        state = new SendMavlinkCommandState(&machine, command);
    }
    auto* const finalState = new QFinalState(&machine);
    state->addTransition(state, &QGCState::advance, finalState);
    state->addTransition(state, &QGCState::error, finalState);
    machine.setInitialState(state);

    QSignalSpy advanceSpy(state, &QGCState::advance);
    QSignalSpy errorSpy(state, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);
    if (!accepted) {
        expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtWarningMsg,
                         QRegularExpression(QStringLiteral("Command failed = ack.result: %1").arg(MAV_RESULT_FAILED)));
    }
    _mockLink->clearReceivedMavCommandCounts();
    machine.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::mediumMs());
    if (!accepted) {
        verifyExpectedLogMessage();
    }

    QCOMPARE(_mockLink->receivedMavCommandCount(command), 1);
    QCOMPARE(advanceSpy.count(), accepted ? 1 : 0);
    QCOMPARE(errorSpy.count(), accepted ? 0 : 1);
}

void SendMavlinkCommandStateTest::_testIgnoresResultThatIsNotItsOwn_data()
{
    QTest::addColumn<bool>("sameCommand");
    QTest::addColumn<bool>("sameComponent");

    QTest::newRow("result of another command") << false << true;
    QTest::newRow("result of the same command sent to another component") << true << false;
    QTest::newRow("refusal of the same command that something else sends to the same component") << true << true;
}

void SendMavlinkCommandStateTest::_testIgnoresResultThatIsNotItsOwn()
{
    QFETCH(bool, sameCommand);
    QFETCH(bool, sameComponent);

    _connectMockLinkNoInitialConnectSequence();
    QVERIFY(_vehicle);

    // The state's own command stays unanswered and is not given up on while the test runs
    TestFixtures::MavCommandAckTimeoutFixture ackTimeout(TestTimeout::longMs());
    _mockLink->clearReceivedMavCommandCounts();

    QGCStateMachine machine(QStringLiteral("TestMachine"), _vehicle);
    auto* const state = new SendMavlinkCommandState(&machine, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE);
    machine.setInitialState(state);

    QSignalSpy advanceSpy(state, &QGCState::advance);
    QSignalSpy errorSpy(state, &QGCState::error);
    machine.start();
    QVERIFY_TRUE_WAIT(_mockLink->receivedMavCommandCount(MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE) == 1,
                      TestTimeout::mediumMs());

    if (sameCommand && sameComponent) {
        // The vehicle refuses the second sender of a command that is pending, before the call returns
        QSignalSpy resultSpy(_vehicle, &Vehicle::mavCommandResult);
        _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE, false /* showError */);
        QCOMPARE(resultSpy.count(), 1);
        QCOMPARE(resultSpy.first().at(4).toInt(), static_cast<int>(Vehicle::MavCmdResultFailureDuplicateCommand));
    } else if (sameCommand) {
        // As the vehicle reports the result of a command that something else has sent to a camera
        emit _vehicle->mavCommandResult(_vehicle->id(), MAV_COMP_ID_CAMERA, MockLink::MAV_CMD_MOCKLINK_NO_RESPONSE,
                                        MAV_RESULT_ACCEPTED, Vehicle::MavCmdResultCommandResultOnly);
    } else {
        QSignalSpy resultSpy(_vehicle, &Vehicle::mavCommandResult);
        _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1, MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED,
                                 false /* showError */);
        QVERIFY_SIGNAL_COUNT_WAIT(resultSpy, 1, TestTimeout::mediumMs());
        QCOMPARE(resultSpy.first().at(2).toInt(), static_cast<int>(MockLink::MAV_CMD_MOCKLINK_ALWAYS_RESULT_ACCEPTED));
    }

    QCOMPARE(advanceSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);
}

void SendMavlinkCommandStateTest::_testUnconfiguredStateFails()
{
    QStateMachine machine;

    expectLogMessage("Utilities.StateMachine.QGCStateMachine", QtCriticalMsg,
                     QRegularExpression("SendMavlinkCommandState not configured"));

    auto* const state = new SendMavlinkCommandState(&machine);
    state->addTransition(state, &QGCState::error, new QFinalState(&machine));
    machine.setInitialState(state);

    QSignalSpy errorSpy(state, &QGCState::error);
    QSignalSpy finishedSpy(&machine, &QStateMachine::finished);

    // Should fail immediately because not configured
    machine.start();
    QVERIFY_SIGNAL_COUNT_WAIT(finishedSpy, 1, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(errorSpy.count(), 1);
}

UT_REGISTER_TEST(SendMavlinkCommandStateTest, TestLabel::Integration, TestLabel::Vehicle, TestLabel::Utilities)
