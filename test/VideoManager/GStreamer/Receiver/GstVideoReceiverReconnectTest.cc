#include "GstVideoReceiverReconnectTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

namespace {

using GstReceiverTest::FileFormat;
using GstReceiverTest::kFirstReconnectDelayMs;
using GstReceiverTest::kNoReconnectWindowMs;
using GstReceiverTest::kShortWatchdogTimeout;
using GstReceiverTest::loopbackUri;
using GstReceiverTest::Receiver;
using GstReceiverTest::State;
using GstReceiverTest::udpListenUri;

}  // namespace

void GstVideoReceiverReconnectTest::_testPermanentFailureStops()
{
    Receiver stream(QStringLiteral("ftp://127.0.0.1:5600/video"), /*autoReconnect=*/true);
    expectLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Unsupported URI scheme")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream stopped:")));
    stream.receiver->setActive(true);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(stream.receiver->state(), State::Stopped);

    // Recording needs a stream that runs.
    stream.receiver->startRecording(QStringLiteral("unused.mp4"), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 2, TestTimeout::mediumMs());
    QVERIFY(stream.recordingSpy.isEmpty());

    // Not even an attempt to reconnect, though autoReconnect is on.
    QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kNoReconnectWindowMs);
    QCOMPARE(stream.errorSpy.count(), 2);
}

void GstVideoReceiverReconnectTest::_testReconnectBacksOffAfterFailedStart()
{
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));
    Receiver stream(loopbackUri(QLatin1StringView("udp265"), portBlocker.localPort()), /*autoReconnect=*/true);

    // The start fails on the bound port; the retry, once it is free, succeeds.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from ")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to start")));
    // Timed from the start, which fails at once: the retry comes at least the first reconnect delay later however
    // late this thread sees the state changes.
    QElapsedTimer clock;
    clock.start();
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Reconnecting));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    portBlocker.close();
    QVERIFY(stream.waitForState(State::Connecting));
    const qint64 retry = clock.elapsed();
    QVERIFY2(retry >= kFirstReconnectDelayMs, qPrintable(QStringLiteral("retried after %1 ms").arg(retry)));
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverReconnectTest::_testStreamLossRestartPolicy_data()
{
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("auto-reconnect") << true;
    QTest::newRow("no auto-reconnect") << false;
}

void GstVideoReceiverReconnectTest::_testStreamLossRestartPolicy()
{
    QFETCH(bool, autoReconnect);

    Receiver stream(udpListenUri(), autoReconnect, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());

    if (!autoReconnect) {
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Video stream stopped:")));
    }
    stream.sender.stop();

    if (autoReconnect) {
        QVERIFY(stream.waitForState(State::Reconnecting));
        QVERIFY(stream.resumeSending());
        QVERIFY(stream.errorSpy.isEmpty());
    } else {
        QVERIFY(stream.waitForState(State::Stopped));
        QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::shortMs());
        verifyExpectedLogMessage();
        stream.stateSpy.clear();
        QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kNoReconnectWindowMs);
    }
}

void GstVideoReceiverReconnectTest::_testUnresponsiveRtspServer_data()
{
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("auto-reconnect") << true;
    QTest::newRow("no auto-reconnect") << false;
}

void GstVideoReceiverReconnectTest::_testUnresponsiveRtspServer()
{
    QFETCH(bool, autoReconnect);

    GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");
    GstTest::FakeRtspServer server;
    QVERIFY(server.listen());

    // Either rtspsrc gives up on the unanswered OPTIONS after its TCP timeout or the first-frame watchdog fires
    // first; which one warns depends on the host's timing, and the receiver's reaction is what is checked.
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from |^Video stream stopped:")));
    Receiver stream(server.uri(), autoReconnect, kShortWatchdogTimeout);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    QVERIFY_TRUE_WAIT(server.connections() == 1, TestTimeout::mediumMs());

    if (autoReconnect) {
        QVERIFY_TRUE_WAIT(server.connections() >= 2, TestTimeout::longMs());
        QVERIFY(stream.errorSpy.isEmpty());
    } else {
        QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::longMs());
        QVERIFY(stream.waitForState(State::Stopped));
        QCOMPARE(server.connections(), 1);
    }
    QVERIFY(stream.streamingSpy.isEmpty());
}

#endif

UT_REGISTER_TEST(GstVideoReceiverReconnectTest, TestLabel::Integration)
