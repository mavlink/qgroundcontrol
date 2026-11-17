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

// Longer than the watchdog takes to give up on a source: the shortest timeout, counted from its first one-second tick
// and checked at the next two.
constexpr int kSilentSourceWindowMs = 3500;

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
    // Qt's precise timers are multimedia timers on Windows, which run up to 1.5% ahead of this clock.
    const qint64 earliestRetry = kFirstReconnectDelayMs * 95 / 100;
    QVERIFY2(retry >= earliestRetry, qPrintable(QStringLiteral("retried after %1 ms").arg(retry)));
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverReconnectTest::_testUdpListensUntilStreamStarts()
{
    // Without auto-reconnect a stream that fails stops, so one that stays Connecting didn't fail.
    Receiver stream(udpListenUri(), /*autoReconnect=*/false, kShortWatchdogTimeout);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    const quint16 port = stream.udpPort();
    QVERIFY(port != 0);

    QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kSilentSourceWindowMs);
    // Still the first session's socket: nothing sent to it meanwhile would have been lost.
    QCOMPARE(stream.udpPort(), port);

    QVERIFY(stream.sender.start(port));
    QVERIFY(stream.waitForState(State::Streaming));
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

void GstVideoReceiverReconnectTest::_testRtspUnauthorizedStops()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");
    GstTest::FakeRtspServer server;
    server.refuseWith(QByteArrayLiteral("401 Unauthorized"));
    QVERIFY(server.listen());

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from ")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream stopped:")));
    Receiver stream(server.uri(QStringLiteral("user:wrong@")), /*autoReconnect=*/true);
    stream.receiver->setActive(true);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY2(stream.errorSpy.constFirst().constFirst().toString().contains(QLatin1String("user name or password")),
             qPrintable(stream.errorSpy.constFirst().constFirst().toString()));
    QVERIFY(stream.waitForState(State::Stopped));

    // Not asked again, though autoReconnect is on: a camera may lock an account out over repeated wrong passwords.
    stream.stateSpy.clear();
    QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kNoReconnectWindowMs);
    QCOMPARE(server.connections(), 1);
}

void GstVideoReceiverReconnectTest::_testRtspWithoutVideoStreamReconnects()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");
    // No stream the receiver sets up: rtspsrc reports that as a codec it can't find, which no codec would fix, while a
    // camera that offers its video later would.
    GstTest::FakeRtspServer server(
        QByteArrayLiteral("v=0\r\n"
                          "o=- 0 0 IN IP4 127.0.0.1\r\n"
                          "s=QGC audio-only test\r\n"
                          "c=IN IP4 127.0.0.1\r\n"
                          "t=0 0\r\n"
                          "m=audio 0 RTP/AVP 97\r\n"
                          "a=rtpmap:97 OPUS/48000/2\r\n"
                          "a=control:trackID=0\r\n"));
    QVERIFY(server.listen());

    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from ")));
    Receiver stream(server.uri(), /*autoReconnect=*/true);
    stream.receiver->setActive(true);
    QVERIFY_TRUE_WAIT(server.connections() >= 2, TestTimeout::mediumMs());
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverReconnectTest::_testFailureMessageNamesCause_data()
{
    QTest::addColumn<bool>("rtsp");
    QTest::addColumn<QString>("debug");
    QTest::addColumn<QString>("cause");

    // The element's error text alone is "Could not get/set settings from/on resource." and "Not found".
    QTest::newRow("udp port in use") << false << QStringLiteral("bind failed") << QStringLiteral("bind failed");
    QTest::newRow("rtsp path not found") << true << QStringLiteral("Not Found \\(404\\)")
                                         << QStringLiteral("RTSP 404 Not Found");
}

void GstVideoReceiverReconnectTest::_testFailureMessageNamesCause()
{
    QFETCH(bool, rtsp);
    QFETCH(QString, debug);
    QFETCH(QString, cause);

    if (rtsp) {
        GSTTEST_SKIP_UNLESS_FACTORIES("rtspsrc");
    }
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));
    GstTest::FakeRtspServer server;
    server.refuseWith(QByteArrayLiteral("404 Not Found"));
    QVERIFY(server.listen());

    // The log has the element's whole debug string, which spans lines; the user what it says of the cause.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("(?s)^GStreamer error from .* debug: .*%1").arg(debug)));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to start|^Video stream stopped:")));
    Receiver stream(rtsp ? server.uri() : loopbackUri(QLatin1StringView("udp265"), portBlocker.localPort()),
                    /*autoReconnect=*/false);
    stream.receiver->setActive(true);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    const QString message = stream.errorSpy.constFirst().constFirst().toString();
    QVERIFY2(message.contains(cause), qPrintable(message));
}

#endif

UT_REGISTER_TEST(GstVideoReceiverReconnectTest, TestLabel::Integration)
