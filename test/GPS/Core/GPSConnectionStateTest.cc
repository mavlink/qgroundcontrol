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
        const auto deadline = state._retryDeadlineMs;
        state.failed();
        QCOMPARE(state._retryDeadlineMs, deadline);
        state._retryDeadlineMs = 0;
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

void GPSConnectionStateTest::_pauseDuringRetryNotification()
{
    GPSConnectionState state;
    state.requestConnect();
    QVERIFY(state.beginAttempt());
    state.failed();
    bool paused = false;
    connect(&state, &GPSConnectionState::changed, &state, [&]() {
        if (!paused && state.state() == GPSConnectionState::Disconnected) {
            paused = true;
            state.pause();
        }
    });
    state.requestConnect();
    QVERIFY(paused);
    QVERIFY(state.paused());
    QVERIFY(!state.active());
    QVERIFY(!state.canAttempt());
    QVERIFY(!state.shouldConnect(true));
}

void GPSConnectionStateTest::_cancelDuringAdmission_data()
{
    QTest::addColumn<bool>("pause");
    QTest::newRow("pause") << true;
    QTest::newRow("stop") << false;
}

void GPSConnectionStateTest::_cancelDuringAdmission()
{
    QFETCH(bool, pause);
    GPSConnectionState state;
    state.requestConnect();
    connect(&state, &GPSConnectionState::changed, &state, [&]() {
        if (state.active() && state.state() == GPSConnectionState::Connecting) {
            if (pause) {
                state.pause();
            } else {
                state.stop();
            }
        }
    });
    bool started = false;
    QVERIFY(!state.startAttempt([&]() {
        started = true;
        return true;
    }));
    QVERIFY(!started);
    QVERIFY(!state.active());
    QCOMPARE(state.state(), GPSConnectionState::Disconnected);
}

void GPSConnectionStateTest::_replacementAdmissionSurvivesRollback()
{
    GPSConnectionState state;
    state.requestConnect();
    bool replaced = false;
    connect(&state, &GPSConnectionState::changed, &state, [&]() {
        if (!replaced && state.state() == GPSConnectionState::Connecting) {
            replaced = true;
            state.stopped();
            QVERIFY(state.startAttempt([]() { return true; }));
        }
    });
    bool oldStarted = false;
    QVERIFY(!state.startAttempt([&]() {
        oldStarted = true;
        return false;
    }));
    QVERIFY(replaced);
    QVERIFY(!oldStarted);
    QCOMPARE(state.state(), GPSConnectionState::Connecting);
}
