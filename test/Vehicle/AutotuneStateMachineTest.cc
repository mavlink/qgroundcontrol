#include "AutotuneStateMachineTest.h"
#include "AutotuneStateMachine.h"
#include "Autotune.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void AutotuneStateMachineTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    Autotune autotune(_vehicle);

    // Access state machine through property methods
    QVERIFY(!autotune.autotuneInProgress());
    QCOMPARE(autotune.autotuneProgress(), 0.0f);
    QVERIFY(!autotune.autotuneStatus().isEmpty());

    _disconnectMockLink();
}

void AutotuneStateMachineTest::_testStartAutotune()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    Autotune autotune(_vehicle);

    QSignalSpy changedSpy(&autotune, &Autotune::autotuneChanged);

    autotune.autotuneRequest();

    // Should be in progress after starting
    QVERIFY(autotune.autotuneInProgress());
    QVERIFY(changedSpy.count() > 0);

    _disconnectMockLink();
}

void AutotuneStateMachineTest::_testProgressTransitions()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    Autotune autotune(_vehicle);

    autotune.autotuneRequest();
    QVERIFY(autotune.autotuneInProgress());

    // Simulate progress updates through the static handlers
    mavlink_command_ack_t ack{};
    ack.result = MAV_RESULT_IN_PROGRESS;

    // Progress to roll phase (20%)
    ack.progress = 20;
    Autotune::progressHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack);
    QVERIFY(autotune.autotuneStatus().contains("roll", Qt::CaseInsensitive));

    // Progress to pitch phase (40%)
    ack.progress = 40;
    Autotune::progressHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack);
    QVERIFY(autotune.autotuneStatus().contains("pitch", Qt::CaseInsensitive));

    // Progress to yaw phase (60%)
    ack.progress = 60;
    Autotune::progressHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack);
    QVERIFY(autotune.autotuneStatus().contains("yaw", Qt::CaseInsensitive));

    // Progress to wait for disarm (95%)
    ack.progress = 95;
    Autotune::progressHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack);
    QVERIFY(autotune.autotuneStatus().contains("disarm", Qt::CaseInsensitive));

    _disconnectMockLink();
}

void AutotuneStateMachineTest::_testFailure()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    Autotune autotune(_vehicle);

    autotune.autotuneRequest();
    QVERIFY(autotune.autotuneInProgress());

    // Simulate failure
    mavlink_command_ack_t ack{};
    ack.result = MAV_RESULT_FAILED;
    Autotune::ackHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack, Vehicle::MavCmdResultCommandResultOnly);

    QVERIFY(!autotune.autotuneInProgress());
    QVERIFY(autotune.autotuneStatus().contains("Failed", Qt::CaseInsensitive));

    _disconnectMockLink();
}

void AutotuneStateMachineTest::_testError()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    Autotune autotune(_vehicle);

    autotune.autotuneRequest();
    QVERIFY(autotune.autotuneInProgress());

    // Simulate error with specific code
    mavlink_command_ack_t ack{};
    ack.result = MAV_RESULT_DENIED;
    Autotune::ackHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack, Vehicle::MavCmdResultCommandResultOnly);

    QVERIFY(!autotune.autotuneInProgress());
    QVERIFY(autotune.autotuneStatus().contains("error", Qt::CaseInsensitive));

    _disconnectMockLink();
}

void AutotuneStateMachineTest::_testFullWorkflow()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    Autotune autotune(_vehicle);

    QSignalSpy changedSpy(&autotune, &Autotune::autotuneChanged);

    autotune.autotuneRequest();
    QVERIFY(autotune.autotuneInProgress());

    mavlink_command_ack_t ack{};
    ack.result = MAV_RESULT_IN_PROGRESS;

    // Walk through all phases
    const QList<int> progressValues = {10, 25, 45, 65, 85, 100};
    for (int progress : progressValues) {
        ack.progress = progress;
        Autotune::progressHandler(&autotune, MAV_COMP_ID_AUTOPILOT1, ack);
    }

    // After 100%, should be complete
    QVERIFY(!autotune.autotuneInProgress());
    QCOMPARE(autotune.autotuneProgress(), 1.0f);
    QVERIFY(autotune.autotuneStatus().contains("Success", Qt::CaseInsensitive));
    QVERIFY(changedSpy.count() > 0);

    _disconnectMockLink();
}
