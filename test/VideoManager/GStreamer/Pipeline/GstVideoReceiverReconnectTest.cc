#include "GstVideoReceiverReconnectTest.h"

#include <array>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

namespace {

using GstReceiverTest::FileFormat;
using GstReceiverTest::Gate;
using GstReceiverTest::kFirstReconnectDelayMs;
using GstReceiverTest::kNoReconnectWindowMs;
using GstReceiverTest::kShortWatchdogTimeout;
using GstReceiverTest::loopbackUri;
using GstReceiverTest::Receiver;
using GstReceiverTest::State;
using GstReceiverTest::udpListenUri;

// How often the receiver checks its worker while a test holds the worker: a stuck one is reported at the seventh
// check, the sixth in a row at which it has run no task. The two windows are counted in checks, so they follow it.
constexpr std::chrono::milliseconds kHeldWorkerHeartbeat{100};
constexpr char kStuckWorkerWarning[] = "^The worker of .* has run none of its tasks";
// A stuck worker's report would be repeated within these seven checks: at each of them, or once six have added up
// again.
constexpr int kRepeatedReportWindowMs = static_cast<int>(7 * kHeldWorkerHeartbeat.count());
// Ends between the third check, the second at which a held worker has run no task since the one before, and the
// fourth.
constexpr int kBusyWorkerWindowMs = static_cast<int>(7 * kHeldWorkerHeartbeat.count() / 2);

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

    // Either rtspsrc gives up on the unanswered OPTIONS after its TCP timeout, a second here rather than the app's
    // five, or the first-frame watchdog fires first; which one warns depends on the host's timing, and the receiver's
    // reaction is what is checked.
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from |^Video stream stopped:")));
    Receiver stream(GstReceiverTimings{.rtspTcpTimeout = std::chrono::seconds(1)}, server.uri(), autoReconnect,
                    kShortWatchdogTimeout);
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
    QVERIFY(!stream.stateSpy.contains(QVariantList{QVariant::fromValue(State::Streaming)}));
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

void GstVideoReceiverReconnectTest::_testStuckWorkerIsReported()
{
    const QRegularExpression stuckWarning(QString::fromLatin1(kStuckWorkerWarning));
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // A decoder that doesn't return holds its streaming thread, and with it the worker that stops the pipeline.
    auto gate = std::make_shared<Gate>();
    // After the receiver, so that a failed check lets the worker go before the receiver waits for it.
    const auto openGate = qScopeGuard([&gate] { gate->release(); });
    QVERIFY(stream.blockDecoder(gate));

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg, stuckWarning);
    stream.receiver->restart();
    // Not before: only a worker that is held may run nothing for intervals this short.
    stream.receiver->setHeartbeatInterval(kHeldWorkerHeartbeat);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    const QString message = stream.errorSpy.constFirst().constFirst().toString();
    QVERIFY2(message.contains(QStringLiteral("stopped responding")), qPrintable(message));
    // The frozen picture goes, as that of a stream that gave up.
    QCOMPARE(stream.receiver->state(), State::Stopped);
    QVERIFY(!stream.receiver->decoding());
    // Once, for as long as the worker is stuck.
    QVERIFY_NO_SIGNAL_WAIT(stream.errorSpy, kRepeatedReportWindowMs);

    // The decoder returns after all: the worker restarts the stream, and the receiver follows it again.
    stream.receiver->setHeartbeatInterval(GstVideoReceiver::kHeartbeatInterval);
    gate->release();
    QVERIFY(stream.resumeSending());
    QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    QCOMPARE(stream.errorSpy.count(), 1);

    // Stuck once more, now stopping a stream that is no longer wanted: reported again.
    gate = std::make_shared<Gate>();
    QVERIFY(stream.blockDecoder(gate));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg, stuckWarning);
    stream.receiver->setActive(false);
    stream.receiver->setHeartbeatInterval(kHeldWorkerHeartbeat);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 2, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::mediumMs());

    // The worker gets to its requests again, and stops when the receiver goes.
    gate->release();
    bool stopped = false;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&stopped](GstElement* pipeline) { stopped = !pipeline; }));
    QVERIFY(stopped);
    QCOMPARE(stream.errorSpy.count(), 2);
}

void GstVideoReceiverReconnectTest::_testStuckWorkerKeepsMovingPicture()
{
    // Reconnects, so that the worker, once it is let go, doesn't give up on the stream this test stops.
    Receiver stream(udpListenUri(), /*autoReconnect=*/true);
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // Stuck elsewhere than in the display, as behind a recorder that doesn't return from a write to a hung network
    // mount: the pipeline goes on decoding without its worker.
    const auto gate = std::make_shared<Gate>();
    const auto openGate = qScopeGuard([gate] { gate->release(); });
    holdWorker(stream.receiver.get(), gate);
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QString::fromLatin1(kStuckWorkerWarning)));
    stream.receiver->setHeartbeatInterval(kHeldWorkerHeartbeat);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    // The user is told, and keeps the picture while its frames arrive. On a busy machine not at once: a check of the
    // worker that no frame came before takes the picture away until one does.
    QVERIFY_TRUE_WAIT(stream.receiver->streaming() && stream.receiver->decoding(), TestTimeout::mediumMs());

    // Nothing reconnects a stream whose worker is stuck: the last frame of one that stops is not left up as its
    // picture.
    stream.sender.stop();
    QVERIFY_TRUE_WAIT(!stream.receiver->streaming() && !stream.receiver->decoding(), TestTimeout::mediumMs());
    QCOMPARE(stream.errorSpy.count(), 1);
}

void GstVideoReceiverReconnectTest::_testBusyWorkerIsNotReported()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // Behind with a row of tasks that each take three checks, as stopping and connecting again to a host that answers
    // late does: twice as long in all as a stuck worker is given, and more of them than it takes their checks without
    // a task run to add up to a report.
    const std::array gates{std::make_shared<Gate>(), std::make_shared<Gate>(), std::make_shared<Gate>(),
                           std::make_shared<Gate>()};
    const auto openGates = qScopeGuard([&gates] {
        for (const std::shared_ptr<Gate>& gate : gates) {
            gate->release();
        }
    });
    for (const std::shared_ptr<Gate>& gate : gates) {
        holdWorker(stream.receiver.get(), gate);
    }

    for (const std::shared_ptr<Gate>& gate : gates) {
        // The worker is through with the task before, and no check ran meanwhile.
        QVERIFY(gate->waitUntilReached(TestTimeout::mediumDuration()));
        // Starts the interval over, so the window ends as far from a check as it can.
        stream.receiver->setHeartbeatInterval(kHeldWorkerHeartbeat);
        QVERIFY_NO_SIGNAL_WAIT(stream.errorSpy, kBusyWorkerWindowMs);
        gate->release();
    }
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* /*pipeline*/) {}));
    QCOMPARE(stream.receiver->state(), State::Streaming);
}

UT_REGISTER_TEST(GstVideoReceiverReconnectTest, TestLabel::Integration)
