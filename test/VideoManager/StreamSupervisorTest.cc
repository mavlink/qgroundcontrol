#include "StreamSupervisorTest.h"

#include <QtTest/QSignalSpy>

#include "StreamSupervisor.h"

namespace {

using namespace std::chrono_literals;

qint64 failSec(StreamSupervisor& supervisor)
{
    const StreamSupervisor::FailureOutcome outcome =
        supervisor.sessionFailed(QStringLiteral("failed"), /*permanent=*/false, /*autoReconnect=*/true);
    return outcome.halted ? -1 : std::chrono::duration_cast<std::chrono::seconds>(outcome.reconnectDelay).count();
}

}  // namespace

void StreamSupervisorTest::_testBackoffSequence()
{
    StreamSupervisor supervisor;
    for (const qint64 expectedSec : {1, 2, 4, 8, 16, 30, 30}) {
        supervisor.sessionStarted();
        QCOMPARE(failSec(supervisor), expectedSec);
    }
    QCOMPARE(supervisor.reconnectAttempts(), 7);
}

void StreamSupervisorTest::_testBackoffRestartsAfterRecovery()
{
    StreamSupervisor supervisor;
    for (int failedStart = 0; failedStart < 3; ++failedStart) {
        (void) failSec(supervisor);
    }

    // The reconnect worked and streamed before the stream failed again.
    supervisor.sessionStarted();
    supervisor.sessionStreaming();
    QCOMPARE(failSec(supervisor), qint64(1));
    // Its own retry failing to start backs off from there.
    QCOMPARE(failSec(supervisor), qint64(2));
}

void StreamSupervisorTest::_testStartOverResetsBackoffAndReconnects()
{
    StreamSupervisor supervisor;
    for (int session = 0; session < 3; ++session) {
        supervisor.sessionStarted();
        supervisor.sessionStreaming();
        // A session counts once, however often it reports streaming.
        supervisor.sessionStreaming();
        QVERIFY(supervisor.uptime() >= 0ms);
        (void) failSec(supervisor);
        QCOMPARE(supervisor.uptime(), 0ms);
    }
    QCOMPARE(supervisor.reconnects(), 2);
    (void) failSec(supervisor);
    QVERIFY(supervisor.reconnectPending());

    supervisor.startOver();
    QVERIFY(!supervisor.reconnectPending());
    QCOMPARE(supervisor.reconnects(), 0);
    QCOMPARE(failSec(supervisor), qint64(1));
}

void StreamSupervisorTest::_testHaltsWithoutAutoReconnect_data()
{
    QTest::addColumn<bool>("permanent");
    QTest::addColumn<bool>("autoReconnect");
    QTest::addColumn<bool>("halted");

    QTest::newRow("transient, autoReconnect") << false << true << false;
    QTest::newRow("transient, no autoReconnect") << false << false << true;
    QTest::newRow("permanent") << true << true << true;
}

void StreamSupervisorTest::_testHaltsWithoutAutoReconnect()
{
    QFETCH(bool, permanent);
    QFETCH(bool, autoReconnect);
    QFETCH(bool, halted);

    StreamSupervisor supervisor;
    supervisor.sessionStarted();
    const StreamSupervisor::FailureOutcome outcome =
        supervisor.sessionFailed(QStringLiteral("gone"), permanent, autoReconnect);
    QCOMPARE(outcome.halted, halted);
    QCOMPARE(supervisor.halted(), halted);
    QCOMPARE(supervisor.reconnectPending(), !halted);
    QCOMPARE(supervisor.lastFailure(), QStringLiteral("gone"));
    QCOMPARE(supervisor.state(), halted ? VideoReceiver::State::Stopped : VideoReceiver::State::Reconnecting);

    // A user start lifts any halt.
    supervisor.startOver();
    QVERIFY(!supervisor.halted());
}

void StreamSupervisorTest::_testAutoReconnectChange()
{
    StreamSupervisor supervisor;
    QCOMPARE(supervisor.autoReconnectChanged(false), StreamSupervisor::AutoReconnectChange::None);

    // Turned off while a reconnect waits: the stream halts, and turned on again it may resume.
    (void) failSec(supervisor);
    QCOMPARE(supervisor.autoReconnectChanged(false), StreamSupervisor::AutoReconnectChange::Halted);
    QVERIFY(supervisor.halted());
    QVERIFY(!supervisor.reconnectPending());
    QCOMPARE(supervisor.autoReconnectChanged(true), StreamSupervisor::AutoReconnectChange::Resumable);
    QVERIFY(!supervisor.halted());

    // A permanent failure stays halted.
    (void) supervisor.sessionFailed(QStringLiteral("unsupported"), /*permanent=*/true, /*autoReconnect=*/true);
    QCOMPARE(supervisor.autoReconnectChanged(true), StreamSupervisor::AutoReconnectChange::None);
    QVERIFY(supervisor.halted());
}

void StreamSupervisorTest::_testStates()
{
    StreamSupervisor supervisor;
    QSignalSpy states(&supervisor, &StreamSupervisor::stateChanged);
    QCOMPARE(supervisor.state(), VideoReceiver::State::Stopped);

    supervisor.sessionStarted();
    supervisor.sessionStreaming();
    // A user restart keeps the running session's state.
    supervisor.startOver();
    (void) failSec(supervisor);
    supervisor.sessionStarted();
    supervisor.stop();
    // Halting after a stop doesn't change what is reported.
    (void) supervisor.sessionFailed(QStringLiteral("gone"), /*permanent=*/false, /*autoReconnect=*/false);

    QList<VideoReceiver::State> reported;
    for (const QList<QVariant>& args : std::as_const(states)) {
        reported.append(args.at(0).value<VideoReceiver::State>());
    }
    const QList<VideoReceiver::State> expected{VideoReceiver::State::Connecting, VideoReceiver::State::Streaming,
                                               VideoReceiver::State::Reconnecting, VideoReceiver::State::Connecting,
                                               VideoReceiver::State::Stopped};
    QCOMPARE(reported, expected);
}

void StreamSupervisorTest::_testReconnectDue()
{
    StreamSupervisor supervisor(nullptr, ExponentialBackoff{10ms, 2, 40ms});
    QSignalSpy due(&supervisor, &StreamSupervisor::reconnectDue);

    const StreamSupervisor::FailureOutcome outcome =
        supervisor.sessionFailed(QStringLiteral("failed"), /*permanent=*/false, /*autoReconnect=*/true);
    QCOMPARE(outcome.reconnectDelay, 10ms);
    QVERIFY(due.wait(TestTimeout::shortMs()));
    QCOMPARE(due.count(), 1);

    // Stopping drops a pending reconnect.
    (void) supervisor.sessionFailed(QStringLiteral("failed"), /*permanent=*/false, /*autoReconnect=*/true);
    supervisor.stop();
    QVERIFY(!due.wait(100));
}

UT_REGISTER_TEST(StreamSupervisorTest, TestLabel::Unit)
