#include "VideoReceiverContractTest.h"

#include <chrono>
#include <memory>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

#include "GStreamerTestHelpers.h"
#include "GstReceiverTimings.h"
#include "GstVideoReceiver.h"
#include "QtMultimedia/QtMultimediaReceiverTestSupport.h"
#include "QtMultimedia/QtMultimediaVideoReceiverTest.h"
#include "QtMultimediaVideoReceiver.h"
#include "VideoReceiver.h"
#include "VideoStreamStats.h"
#include "VideoTestHelpers.h"

#ifdef QGC_FFMPEG_VIDEO
#include "FFmpeg/FFmpegVideoReceiverStreamTest.h"
#include "FFmpegVideoReceiver.h"
#endif

namespace {

enum class ContractBackend
{
    GStreamer,
    QtMultimedia,
#ifdef QGC_FFMPEG_VIDEO
    FFmpeg,
#endif
};

// The stream GstTest::FixtureRtpSender sends.
constexpr QSize kContractFrameSize{320, 240};
// The shortest stream timeout, so that a lost stream is noticed in about a second.
constexpr std::chrono::seconds kContractTimeout{1};
// A window that shows a reconnect would have started: the first one follows a failure after a second.
constexpr int kContractNoReconnectWindowMs = 1200;

void addBackendRows()
{
    QTest::addColumn<int>("backend");

    QTest::newRow("GStreamer") << int(ContractBackend::GStreamer);
    QTest::newRow("Qt Multimedia") << int(ContractBackend::QtMultimedia);
#ifdef QGC_FFMPEG_VIDEO
    QTest::newRow("FFmpeg") << int(ContractBackend::FFmpeg);
#endif
}

std::unique_ptr<VideoReceiver> createContractReceiver(ContractBackend backend)
{
    if (backend == ContractBackend::QtMultimedia) {
        auto receiver = std::make_unique<QtMultimediaVideoReceiver>();
        QtMultimediaVideoReceiverTest::sampleOften(*receiver);
        return receiver;
    }
#ifdef QGC_FFMPEG_VIDEO
    if (backend == ContractBackend::FFmpeg) {
        auto receiver = std::make_unique<FFmpegVideoReceiver>();
        FFmpegVideoReceiverStreamTest::sampleOften(*receiver);
        return receiver;
    }
#endif
    // Looks at its stream ten times a second rather than once: the cases wait for its statistics, and for it to
    // notice a stream that stopped.
    auto receiver = std::make_unique<GstVideoReceiver>(
        nullptr, GstReceiverTimings{.watchdogInterval = std::chrono::milliseconds(100)});
    // The CPU display path needs no GPU on the test host.
    receiver->setDisplayConfig({.conversionElement = {}, .disablePixelAspectRatio = false, .gpuZeroCopy = false});
    return receiver;
}

/// Longer than a receiver that gave up on a port nobody sends to would take to say so. GStreamer: the timeout, counted
/// from the worker's first look at the stream and checked at the first one after it is over. Qt Multimedia: the
/// timeout and the five seconds FFmpeg has for a stream's first frame, after which its player gives up and the
/// receiver listens again. FFmpeg: two timeouts and a half, as its watchdog would fail a silent session after one.
int silentPortWindowMs(ContractBackend backend)
{
    switch (backend) {
        case ContractBackend::QtMultimedia:
            return 6500;
#ifdef QGC_FFMPEG_VIDEO
        case ContractBackend::FFmpeg:
            return 2500;
#endif
        case ContractBackend::GStreamer:
            break;
    }
    return 1500;
}

/// A receiver of one backend that listens on a free UDP port of loopback, with spies on its signals, a VideoOutput in
/// a shown window, and a sender of the test stream.
struct ContractStream
{
    ContractStream(ContractBackend backend, bool autoReconnect)
        : output(new QQuickVideoOutput(window.contentItem()))
        , receiver(createContractReceiver(backend))
        , stateSpy(receiver.get(), &VideoReceiver::stateChanged)
        , errorSpy(receiver.get(), &VideoReceiver::errorOccurred)
        , recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted)
        , recordingFinishedSpy(receiver.get(), &VideoReceiver::recordingFinished)
        , port(VideoTestHelpers::freeUdpPort())
    {
        window.resize(kContractFrameSize);
        output->setSize(QSizeF(kContractFrameSize));
        window.show();
        receiver->setName(QStringLiteral("test"));
        receiver->setConfig({.uri = QStringLiteral("udp://127.0.0.1:%1").arg(port),
                             .timeout = kContractTimeout,
                             .lowLatency = true,
                             .autoReconnect = autoReconnect});
        receiver->setVideoOutput(output);
    }

    ContractStream(const ContractStream&) = delete;
    ContractStream& operator=(const ContractStream&) = delete;

    [[nodiscard]] bool waitForState(VideoReceiver::State state) const
    {
        return UnitTest::waitForCondition([this, state] { return receiver->state() == state; },
                                          TestTimeout::longDuration(), u"receiver state");
    }

    /// The receiver has been in @p state since the spy was cleared.
    bool passedThrough(VideoReceiver::State state) const
    {
        return stateSpy.contains(QVariantList{QVariant::fromValue(state)});
    }

    /// Frames of the stream reach the output and the statistics measure them.
    [[nodiscard]] bool waitForDisplay() const
    {
        return UnitTest::waitForCondition(
            [this] {
                const VideoStreamStats stats = receiver->stats();
                return receiver->streaming() && receiver->decoding() && (receiver->videoSize() == kContractFrameSize) &&
                       stats.valid && (stats.fps > 0);
            },
            TestTimeout::longDuration(), u"display");
    }

    /// Sends the stream to the receiver once it listens, until it is displayed.
    [[nodiscard]] bool sendUntilDisplayed()
    {
        return waitForState(VideoReceiver::State::Connecting) && sender.start(port) && waitForDisplay();
    }

    [[nodiscard]] bool startStreaming()
    {
        receiver->setActive(true);
        return sendUntilDisplayed();
    }

    QQuickWindow window;
    QQuickVideoOutput* const output;
    std::unique_ptr<VideoReceiver> receiver;
    QSignalSpy stateSpy;
    QSignalSpy errorSpy;
    QSignalSpy recordingStartedSpy;
    QSignalSpy recordingFinishedSpy;
    const quint16 port;
    GstTest::FixtureRtpSender sender;
};

}  // namespace

void VideoReceiverContractTest::_ignoreBackendLogs()
{
    QFETCH(int, backend);
    if (static_cast<ContractBackend>(backend) != ContractBackend::QtMultimedia) {
        return;
    }
    for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::playerLogs()) {
        ignoreLogMessage(log.category, log.type, log.pattern);
    }
    for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::encoderFallbackLogs()) {
        ignoreLogMessage(log.category, log.type, log.pattern);
    }
}

void VideoReceiverContractTest::_testListensUntilStreamStarts_data()
{
    addBackendRows();
}

void VideoReceiverContractTest::_testListensUntilStreamStarts()
{
    QFETCH(int, backend);
    const auto contractBackend = static_cast<ContractBackend>(backend);
    _ignoreBackendLogs();

    // Without auto-reconnect a stream that fails stops, so one that stays Connecting didn't fail.
    ContractStream stream(contractBackend, /*autoReconnect=*/false);
    QVERIFY(stream.port != 0);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(VideoReceiver::State::Connecting));
    stream.stateSpy.clear();
    QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, silentPortWindowMs(contractBackend));
    QVERIFY(stream.errorSpy.isEmpty());

    QVERIFY(stream.sendUntilDisplayed());
    QVERIFY(stream.errorSpy.isEmpty());
}

void VideoReceiverContractTest::_testIncompleteAddressStops_data()
{
    QTest::addColumn<int>("backend");
    QTest::addColumn<QString>("uri");

    QTest::newRow("GStreamer, TCP without a port")
        << int(ContractBackend::GStreamer) << QStringLiteral("tcp://127.0.0.1");
    QTest::newRow("GStreamer, UDP without a port")
        << int(ContractBackend::GStreamer) << QStringLiteral("mpegts://127.0.0.1");
    QTest::newRow("Qt Multimedia, TCP without a port")
        << int(ContractBackend::QtMultimedia) << QStringLiteral("tcp://127.0.0.1");
    QTest::newRow("Qt Multimedia, UDP without a port")
        << int(ContractBackend::QtMultimedia) << QStringLiteral("mpegts://127.0.0.1");
#ifdef QGC_FFMPEG_VIDEO
    QTest::newRow("FFmpeg, TCP without a port") << int(ContractBackend::FFmpeg) << QStringLiteral("tcp://127.0.0.1");
    QTest::newRow("FFmpeg, UDP without a port") << int(ContractBackend::FFmpeg) << QStringLiteral("mpegts://127.0.0.1");
#endif
}

void VideoReceiverContractTest::_testIncompleteAddressStops()
{
    QFETCH(int, backend);
    QFETCH(QString, uri);
    const auto contractBackend = static_cast<ContractBackend>(backend);
    _ignoreBackendLogs();

    ContractStream stream(contractBackend, /*autoReconnect=*/true);
    VideoReceiver::Config config = stream.receiver->config();
    config.uri = uri;
    stream.receiver->setConfig(config);

    const bool gstreamer = (contractBackend == ContractBackend::GStreamer);
    if (gstreamer) {
        expectLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Invalid port in")));
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Video stream stopped:")));
#ifdef QGC_FFMPEG_VIDEO
    } else if (contractBackend == ContractBackend::FFmpeg) {
        expectLogMessage("Video.FFmpegVideoReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("can't play")));
#endif
    } else {
        expectLogMessage("Video.QtMultimediaVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("can't play")));
    }
    stream.receiver->setActive(true);

    // Connecting again can't complete the address: the receiver stops at once, though it reconnects after a failure.
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    if (gstreamer) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Stopped);
    QVERIFY(!stream.passedThrough(VideoReceiver::State::Reconnecting));

    // A stream that doesn't run takes no recording.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    stream.receiver->startRecording(dir.filePath(QStringLiteral("stream.mkv")), VideoReceiver::FileFormat::Mkv);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 2, TestTimeout::mediumMs());
    QVERIFY(!stream.receiver->recording());
    QVERIFY(QDir(dir.path()).isEmpty());
}

void VideoReceiverContractTest::_testStreamLossRestartPolicy_data()
{
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("GStreamer, auto-reconnect") << int(ContractBackend::GStreamer) << true;
    QTest::newRow("GStreamer, no auto-reconnect") << int(ContractBackend::GStreamer) << false;
    QTest::newRow("Qt Multimedia, auto-reconnect") << int(ContractBackend::QtMultimedia) << true;
    QTest::newRow("Qt Multimedia, no auto-reconnect") << int(ContractBackend::QtMultimedia) << false;
#ifdef QGC_FFMPEG_VIDEO
    QTest::newRow("FFmpeg, auto-reconnect") << int(ContractBackend::FFmpeg) << true;
    QTest::newRow("FFmpeg, no auto-reconnect") << int(ContractBackend::FFmpeg) << false;
#endif
}

void VideoReceiverContractTest::_testStreamLossRestartPolicy()
{
    QFETCH(int, backend);
    QFETCH(bool, autoReconnect);
    const auto contractBackend = static_cast<ContractBackend>(backend);
    _ignoreBackendLogs();

    ContractStream stream(contractBackend, autoReconnect);
    QVERIFY(stream.startStreaming());

    // The GStreamer receiver logs the failure it stops at; the others only report it.
    const bool warnsOfStop = !autoReconnect && (contractBackend == ContractBackend::GStreamer);
    if (warnsOfStop) {
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Video stream stopped:")));
    }
    stream.stateSpy.clear();
    stream.sender.stop();

    if (autoReconnect) {
        // No frame for the timeout is a failure; the receiver connects again without telling the user.
        QVERIFY(stream.waitForState(VideoReceiver::State::Reconnecting));
        QVERIFY(!stream.receiver->decoding());
        QVERIFY(!stream.receiver->stats().valid);

        QVERIFY(stream.sendUntilDisplayed());
        QCOMPARE(stream.receiver->stats().reconnects, 1);
        QVERIFY(stream.errorSpy.isEmpty());
        return;
    }

    QVERIFY(stream.waitForState(VideoReceiver::State::Stopped));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::shortMs());
    if (warnsOfStop) {
        verifyExpectedLogMessage();
    }
    QVERIFY(!stream.passedThrough(VideoReceiver::State::Reconnecting));
    stream.stateSpy.clear();
    QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kContractNoReconnectWindowMs);
    QCOMPARE(stream.errorSpy.count(), 1);
}

void VideoReceiverContractTest::_testRestartReopensPort_data()
{
    addBackendRows();
}

void VideoReceiverContractTest::_testRestartReopensPort()
{
    QFETCH(int, backend);
    _ignoreBackendLogs();

    ContractStream stream(static_cast<ContractBackend>(backend), /*autoReconnect=*/true);
    QVERIFY(stream.startStreaming());

    // The new session binds the port once the old one let it go: no failed attempt, no reconnect. A restart counts
    // reconnects anew.
    stream.stateSpy.clear();
    stream.receiver->restart();
    QVERIFY_TRUE_WAIT(stream.passedThrough(VideoReceiver::State::Connecting), TestTimeout::mediumMs());
    QVERIFY(stream.waitForDisplay());
    QVERIFY(!stream.passedThrough(VideoReceiver::State::Reconnecting));
    QCOMPARE(stream.receiver->stats().reconnects, 0);
    QVERIFY(stream.errorSpy.isEmpty());
}

void VideoReceiverContractTest::_testDeactivateEndsRecording_data()
{
    addBackendRows();
}

void VideoReceiverContractTest::_testDeactivateEndsRecording()
{
    QFETCH(int, backend);
    if (static_cast<ContractBackend>(backend) == ContractBackend::GStreamer) {
        GSTTEST_SKIP_UNLESS_FACTORIES("matroskamux");
    }
    _ignoreBackendLogs();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ContractStream stream(static_cast<ContractBackend>(backend), /*autoReconnect=*/false);
    QVERIFY(stream.startStreaming());

    stream.receiver->startRecording(dir.filePath(QStringLiteral("stream.mkv")), VideoReceiver::FileFormat::Mkv);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::longMs());
    QVERIFY(stream.receiver->recording());
    const QString file = stream.recordingStartedSpy.constFirst().constFirst().toString();

    // Stopping the stream ends its recording in good order, and leaves no frames, statistics or state behind.
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::longMs());
    QCOMPARE(stream.recordingFinishedSpy.constFirst().at(0).toString(), file);
    QVERIFY2(stream.recordingFinishedSpy.constFirst().at(1).toString().isEmpty(),
             qPrintable(stream.recordingFinishedSpy.constFirst().at(1).toString()));
    QVERIFY(QFileInfo(file).size() > 0);
    QVERIFY(stream.waitForState(VideoReceiver::State::Stopped));
    QVERIFY_TRUE_WAIT(!stream.receiver->recording() && !stream.receiver->decoding() && !stream.receiver->stats().valid,
                      TestTimeout::shortMs());

    // Activating again doesn't bring the recording back.
    QVERIFY(stream.startStreaming());
    QVERIFY(!stream.receiver->recording());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
    QVERIFY(stream.errorSpy.isEmpty());
}

void VideoReceiverContractTest::_testStreamStoppedForGoodEndsRecording_data()
{
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("autoReconnectTurnedOff");

    QTest::newRow("GStreamer, stream lost") << int(ContractBackend::GStreamer) << false;
    QTest::newRow("GStreamer, auto-reconnect turned off") << int(ContractBackend::GStreamer) << true;
    QTest::newRow("Qt Multimedia, stream lost") << int(ContractBackend::QtMultimedia) << false;
    QTest::newRow("Qt Multimedia, auto-reconnect turned off") << int(ContractBackend::QtMultimedia) << true;
#ifdef QGC_FFMPEG_VIDEO
    QTest::newRow("FFmpeg, stream lost") << int(ContractBackend::FFmpeg) << false;
    QTest::newRow("FFmpeg, auto-reconnect turned off") << int(ContractBackend::FFmpeg) << true;
#endif
}

void VideoReceiverContractTest::_testStreamStoppedForGoodEndsRecording()
{
    QFETCH(int, backend);
    QFETCH(bool, autoReconnectTurnedOff);
    const auto contractBackend = static_cast<ContractBackend>(backend);
    const bool gstreamer = (contractBackend == ContractBackend::GStreamer);
    if (gstreamer) {
        GSTTEST_SKIP_UNLESS_FACTORIES("matroskamux");
    }
    _ignoreBackendLogs();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    // Either the stream is lost with auto-reconnect off, or it is turned off while the stream waits to reconnect.
    ContractStream stream(contractBackend, /*autoReconnect=*/autoReconnectTurnedOff);
    QVERIFY(stream.startStreaming());
    stream.receiver->startRecording(dir.filePath(QStringLiteral("stream.mkv")), VideoReceiver::FileFormat::Mkv);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::longMs());
    const QString file = stream.recordingStartedSpy.constFirst().constFirst().toString();

    if (gstreamer) {
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Video stream stopped:")));
    }
    stream.sender.stop();
    if (autoReconnectTurnedOff) {
        QVERIFY(stream.waitForState(VideoReceiver::State::Reconnecting));
        VideoReceiver::Config config = stream.receiver->config();
        config.autoReconnect = false;
        stream.receiver->setConfig(config);
    }

    // The receiver says that the stream stopped, and ends the recording rather than have it wait for frames that
    // won't come.
    QVERIFY(stream.waitForState(VideoReceiver::State::Stopped));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::shortMs());
    if (gstreamer) {
        verifyExpectedLogMessage();
    }
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::longMs());
    QCOMPARE(stream.recordingFinishedSpy.constFirst().at(0).toString(), file);
    QVERIFY(QFileInfo(file).size() > 0);
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());

    // It takes no other one either.
    stream.receiver->startRecording(dir.filePath(QStringLiteral("second.mkv")), VideoReceiver::FileFormat::Mkv);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 2, TestTimeout::mediumMs());
    QVERIFY(!stream.receiver->recording());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
}

void VideoReceiverContractTest::_testRecordingBeforeFirstFrame_data()
{
    addBackendRows();
}

void VideoReceiverContractTest::_testRecordingBeforeFirstFrame()
{
    QFETCH(int, backend);
    if (static_cast<ContractBackend>(backend) == ContractBackend::GStreamer) {
        GSTTEST_SKIP_UNLESS_FACTORIES("matroskamux");
    }
    _ignoreBackendLogs();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ContractStream stream(static_cast<ContractBackend>(backend), /*autoReconnect=*/false);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(VideoReceiver::State::Connecting));

    // Nobody sends the stream yet: the recording is taken on and waits for it, with no file to tell of.
    stream.receiver->startRecording(dir.filePath(QStringLiteral("stream.mkv")), VideoReceiver::FileFormat::Mkv);
    QVERIFY_TRUE_WAIT(stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.recordingStartedSpy.isEmpty());

    // It ends when asked to, and leaves nothing behind.
    stream.receiver->stopRecording();
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::mediumMs());
    QVERIFY(stream.recordingStartedSpy.isEmpty());
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QVERIFY(QDir(dir.path()).isEmpty());
    QVERIFY(stream.errorSpy.isEmpty());
}

UT_REGISTER_TEST(VideoReceiverContractTest, TestLabel::Integration)
