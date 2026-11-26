#include "VideoReceiverContractTest.h"

#include <chrono>
#include <memory>

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

namespace {

enum class ContractBackend
{
    GStreamer,
    QtMultimedia,
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
}

std::unique_ptr<VideoReceiver> createContractReceiver(ContractBackend backend)
{
    if (backend == ContractBackend::QtMultimedia) {
        auto receiver = std::make_unique<QtMultimediaVideoReceiver>();
        QtMultimediaVideoReceiverTest::sampleOften(*receiver);
        return receiver;
    }
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
/// receiver listens again.
int silentPortWindowMs(ContractBackend backend)
{
    return (backend == ContractBackend::QtMultimedia) ? 6500 : 1500;
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

void VideoReceiverContractTest::_testStreamLossRestartPolicy_data()
{
    QTest::addColumn<int>("backend");
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("GStreamer, auto-reconnect") << int(ContractBackend::GStreamer) << true;
    QTest::newRow("GStreamer, no auto-reconnect") << int(ContractBackend::GStreamer) << false;
    QTest::newRow("Qt Multimedia, auto-reconnect") << int(ContractBackend::QtMultimedia) << true;
    QTest::newRow("Qt Multimedia, no auto-reconnect") << int(ContractBackend::QtMultimedia) << false;
}

void VideoReceiverContractTest::_testStreamLossRestartPolicy()
{
    QFETCH(int, backend);
    QFETCH(bool, autoReconnect);
    const auto contractBackend = static_cast<ContractBackend>(backend);
    _ignoreBackendLogs();

    ContractStream stream(contractBackend, autoReconnect);
    QVERIFY(stream.startStreaming());

    // The GStreamer receiver logs the failure it stops at; the Qt Multimedia one only reports it.
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
    QVERIFY(stream.errorSpy.isEmpty());
}

UT_REGISTER_TEST(VideoReceiverContractTest, TestLabel::Integration)
