#include "GPSConnectionStateTest.h"

#include "GPSConnectionState.h"

void GPSConnectionStateTest::_intentAndPause()
{
    GPSConnectionState state;
    QVERIFY(!state.updateIntent(false));
    QVERIFY(state.updateIntent(true));
    QVERIFY(state.active());
    QCOMPARE(state.state(), GPSConnectionState::Disconnected);
    state.pause();
    QVERIFY(state.paused());
    QVERIFY(!state.updateIntent(true));
    state.requestConnect();
    QVERIFY(!state.paused());
    QVERIFY(state.updateIntent(false));
    state.resetIntent();
    QVERIFY(!state.updateIntent(false));
    QVERIFY(state.updateIntent(true));
    state.stop();
    QVERIFY(!state.active());
}

void GPSConnectionStateTest::_lifecycleAndRetry()
{
    GPSConnectionState state;
    QVERIFY(!state.beginAttempt());
    state.requestConnect();
    QVERIFY(state.beginAttempt());
    QCOMPARE(state.state(), GPSConnectionState::Connecting);
    QVERIFY(!state.beginAttempt());
    state.configuring();
    QCOMPARE(state.state(), GPSConnectionState::Configuring);
    state.ready();
    QCOMPARE(state.state(), GPSConnectionState::Ready);
    for (int delay : {1000, 2000, 4000, 8000, 16000, 30000, 30000}) {
        state.failed();
        QCOMPARE(state.state(), GPSConnectionState::Retrying);
        QCOMPARE(state._retryDelayMs, delay);
        QVERIFY(!state.canAttempt());
        const auto deadline = state._retryDeadline.deadline();
        state.failed();
        QCOMPARE(state._retryDeadline.deadline(), deadline);
        state._retryDeadline.setRemainingTime(0);
        QVERIFY(state.beginAttempt());
    }
    state.ready();
    state.failed();
    QCOMPARE(state._retryDelayMs, 1000);
    state.requestConnect();
    QVERIFY(state.beginAttempt());
    state.failed();
    state.resetIntent();
    QVERIFY(state.updateIntent(true));
    QVERIFY(state.beginAttempt());
    state.pause();
    QVERIFY(!state.canAttempt());
}

void GPSConnectionStateTest::_stoppingBlocksAttempts()
{
    GPSConnectionState state;
    state.requestConnect();
    QVERIFY(state.beginAttempt());
    state.stopping();
    state.requestConnect();
    QVERIFY(!state.beginAttempt());
    state.configuring();
    state.ready();
    state.failed();
    QCOMPARE(state.state(), GPSConnectionState::Stopping);
    state.stopped();
    QVERIFY(state.beginAttempt());
}

UT_REGISTER_TEST(GPSConnectionStateTest, TestLabel::Unit)
