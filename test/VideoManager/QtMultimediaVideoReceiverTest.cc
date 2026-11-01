#include "QtMultimediaVideoReceiverTest.h"

#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QMediaPlayer>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "QtMultimediaVideoReceiver.h"
#include "VideoTestHelpers.h"

#ifdef QGC_GST_STREAMING
#include "MockVideoStreamServer.h"
#endif

namespace {

using State = VideoReceiver::State;

/// A free UDP port, for a stream the receiver listens for, with the next one free for its RTCP. Below the ephemeral
/// ranges (Windows 49152+, Linux 32768+), which hand out ports in turn on Windows: the server's own socket would take
/// the next one.
quint16 freeUdpPort()
{
    constexpr int kFirst = 20000;
    constexpr int kPairs = 6000;
    const int start = static_cast<int>(QRandomGenerator::global()->bounded(kPairs));
    for (int i = 0; i < kPairs; i++) {
        const auto port = static_cast<quint16>(kFirst + (2 * ((start + i) % kPairs)));
        QUdpSocket rtp;
        QUdpSocket rtcp;
        if (rtp.bind(QHostAddress::AnyIPv4, port) && rtcp.bind(QHostAddress::AnyIPv4, port + 1)) {
            return port;
        }
    }
    return 0;
}

/// A QtMultimediaVideoReceiver playing @p uri into a VideoOutput, with spies on its signals.
struct ReceiverUnderTest
{
    explicit ReceiverUnderTest(const QString& uri, bool autoReconnect = false,
                               std::chrono::seconds timeout = std::chrono::seconds(2))
        : output(VideoTestHelpers::createVideoOutput(engine))
        , receiver(std::make_unique<QtMultimediaVideoReceiver>())
        , stateSpy(receiver.get(), &VideoReceiver::stateChanged)
        , errorSpy(receiver.get(), &VideoReceiver::errorOccurred)
    {
        receiver->setName(QStringLiteral("test"));
        receiver->setConfig({.uri = uri, .timeout = timeout, .lowLatency = true, .autoReconnect = autoReconnect});
        receiver->setVideoOutput(output.get());
    }

    [[nodiscard]] bool waitForState(State state, std::chrono::milliseconds timeout = TestTimeout::longDuration()) const
    {
        return UnitTest::waitForCondition([this, state] { return receiver->state() == state; }, timeout,
                                          u"receiver state");
    }

    /// Frames reach the output and the stats measure them.
    [[nodiscard]] bool waitForDisplay() const
    {
        return UnitTest::waitForCondition(
            [this] { return receiver->decoding() && receiver->videoSize().isValid() && receiver->stats().valid; },
            TestTimeout::longDuration(), u"display");
    }

    QQmlEngine engine;
    std::unique_ptr<QQuickItem> output;
    std::unique_ptr<QtMultimediaVideoReceiver> receiver;
    QSignalSpy stateSpy;
    QSignalSpy errorSpy;
};

}  // namespace

void QtMultimediaVideoReceiverTest::_ignorePlayerLogs()
{
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Cannot set loops for non-seekable source$")));
    ignoreLogMessage("qt.multimedia.playbackengine.codec", QtInfoMsg,
                     QRegularExpression(QStringLiteral("No HW decoder found")));
}

void QtMultimediaVideoReceiverTest::_ignoreEncoderFallbacks()
{
    ignoreLogMessage("qt.multimedia.ffmpeg.encoder", QtInfoMsg,
                     QRegularExpression(QStringLiteral("^Recording new media")));
    ignoreLogMessage(
        "default", QtWarningMsg,
        QRegularExpression(QStringLiteral("^(Cannot find software target format|Could not find target format)")));
    ignoreLogMessage("qt.multimedia.ffmpeg.videoencoder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No valid video codecs found")));
    ignoreLogMessage("Video.QtVideoRecorder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .* \"Could not initialize encoder\"$")));
}

void QtMultimediaVideoReceiverTest::_testPlayerSource_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("sdpEncoding");
    QTest::addColumn<bool>("rtspOverTcp");

    QTest::newRow("rtsp") << QStringLiteral("rtsp://192.168.42.1:554/live")
                          << QStringLiteral("rtsp://192.168.42.1:554/live") << QString() << false;
    QTest::newRow("rtspt") << QStringLiteral("rtspt://192.168.42.1:554/live")
                           << QStringLiteral("rtsp://192.168.42.1:554/live") << QString() << true;
    QTest::newRow("rtsps") << QStringLiteral("rtsps://cam/live") << QStringLiteral("rtsps://cam/live") << QString()
                           << false;
    QTest::newRow("tcp") << QStringLiteral("tcp://192.168.143.200:3001") << QStringLiteral("tcp://192.168.143.200:3001")
                         << QString() << false;
    QTest::newRow("mpegts") << QStringLiteral("mpegts://0.0.0.0:5600") << QStringLiteral("udp://0.0.0.0:5600")
                            << QString() << false;
    QTest::newRow("udp h264") << QStringLiteral("udp://0.0.0.0:5600") << QStringLiteral("stream.sdp")
                              << QStringLiteral("H264") << false;
    QTest::newRow("udp h265") << QStringLiteral("udp265://0.0.0.0:5601") << QStringLiteral("stream.sdp")
                              << QStringLiteral("H265") << false;
}

void QtMultimediaVideoReceiverTest::_testPlayerSource()
{
    QFETCH(QString, uri);
    QFETCH(QString, url);
    QFETCH(QString, sdpEncoding);
    QFETCH(bool, rtspOverTcp);

    const std::optional<QtMultimediaVideoReceiver::Source> source = QtMultimediaVideoReceiver::playerSource(uri);
    QVERIFY(source);
    QCOMPARE(source->url, QUrl(url));
    QCOMPARE(source->rtspOverTcp, rtspOverTcp);
    if (sdpEncoding.isEmpty()) {
        QVERIFY(source->sdp.isEmpty());
        return;
    }
    // RTP over UDP: an SDP description of payload type 96 at the URI's port.
    const QString sdp = QString::fromLatin1(source->sdp);
    const QString port = QString::number(QUrl(uri).port());
    QVERIFY2(sdp.contains(QStringLiteral("m=video %1 RTP/AVP 96").arg(port)), qPrintable(sdp));
    QVERIFY2(sdp.contains(QStringLiteral("a=rtpmap:96 %1/90000").arg(sdpEncoding)), qPrintable(sdp));
    QVERIFY2(sdp.contains(QStringLiteral("c=IN IP4 0.0.0.0")), qPrintable(sdp));
}

void QtMultimediaVideoReceiverTest::_testUnplayableUri()
{
    QVERIFY(!QtMultimediaVideoReceiver::playerSource(QString()));
    QVERIFY(!QtMultimediaVideoReceiver::playerSource(QStringLiteral("0.0.0.0:5600")));
    QVERIFY(!QtMultimediaVideoReceiver::playerSource(QStringLiteral("udp://0.0.0.0")));

    ReceiverUnderTest stream(QStringLiteral("udp://0.0.0.0"));
    expectLogMessage("Video.QtMultimediaVideoReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("can't play")));
    stream.receiver->setActive(true);
    verifyExpectedLogMessage();
    QCOMPARE(stream.receiver->state(), State::Stopped);
    QCOMPARE(stream.errorSpy.count(), 1);
}

void QtMultimediaVideoReceiverTest::_testRecordingNeedsActiveStream()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file = dir.filePath(QStringLiteral("stream.mkv"));

    ReceiverUnderTest stream(QStringLiteral("udp://127.0.0.1:%1").arg(freeUdpPort()));
    expectLogMessage("Video.VideoReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("not active")));
    stream.receiver->startRecording(file, VideoReceiver::FileFormat::Mkv);
    verifyExpectedLogMessage();
    QCOMPARE(stream.errorSpy.count(), 1);
    QVERIFY(!stream.receiver->recording());
    QVERIFY(!QFile::exists(file));
}

void QtMultimediaVideoReceiverTest::_testRecordingTimes_data()
{
    QTest::addColumn<qint64>("lastRecordedUs");
    QTest::addColumn<qint64>("lastStreamUs");
    QTest::addColumn<qint64>("streamUs");
    QTest::addColumn<qint64>("arrivalUs");
    QTest::addColumn<qint64>("expectedUs");

    QTest::newRow("first frame") << qint64(-1) << qint64(-1) << qint64(5000000) << qint64(0) << qint64(0);
    QTest::newRow("stream spacing") << qint64(100000) << qint64(5000000) << qint64(5033333) << qint64(140000)
                                    << qint64(133333);
    // Frames the GUI thread took together keep the stream's spacing.
    QTest::newRow("arrived together") << qint64(100000) << qint64(5000000) << qint64(5033333) << qint64(100010)
                                      << qint64(133333);
    // A jump in the stream's times, or a frame without one, goes by its arrival...
    QTest::newRow("stream jumped") << qint64(100000) << qint64(5000000) << qint64(90000000) << qint64(400000)
                                   << qint64(400000);
    QTest::newRow("stream went back") << qint64(100000) << qint64(5000000) << qint64(0) << qint64(400000)
                                      << qint64(400000);
    QTest::newRow("no stream time") << qint64(100000) << qint64(5000000) << qint64(-1) << qint64(140000)
                                    << qint64(140000);
    // ...but never sooner than a millisecond after the last frame, which the encoder couldn't tell apart.
    QTest::newRow("no time to tell apart")
        << qint64(100000) << qint64(-1) << qint64(-1) << qint64(100000) << qint64(101000);
    QTest::newRow("same stream time") << qint64(100000) << qint64(5000000) << qint64(5000000) << qint64(100000)
                                      << qint64(101000);
}

void QtMultimediaVideoReceiverTest::_testRecordingTimes()
{
    QFETCH(qint64, lastRecordedUs);
    QFETCH(qint64, lastStreamUs);
    QFETCH(qint64, streamUs);
    QFETCH(qint64, arrivalUs);
    QFETCH(qint64, expectedUs);

    QCOMPARE(QtMultimediaVideoReceiver::recordingTimeUs(lastRecordedUs, lastStreamUs, streamUs, arrivalUs), expectedUs);
}

#ifdef QGC_GST_STREAMING

void QtMultimediaVideoReceiverTest::_testPlaysMockStreams_data()
{
    QTest::addColumn<int>("streamType");

    using StreamType = MockVideoStreamServer::StreamType;
    QTest::newRow("rtp h264 udp") << int(StreamType::RtpUdpH264);
    QTest::newRow("rtp h265 udp") << int(StreamType::RtpUdpH265);
    QTest::newRow("mpeg-ts udp") << int(StreamType::MpegTsUdp);
    QTest::newRow("mpeg-ts tcp") << int(StreamType::MpegTsTcp);
    QTest::newRow("rtsp") << int(StreamType::RtspH264);
}

void QtMultimediaVideoReceiverTest::_testPlaysMockStreams()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    QFETCH(int, streamType);

    using StreamType = MockVideoStreamServer::StreamType;
    const auto type = static_cast<StreamType>(streamType);
    if (!MockVideoStreamServer::isStreamTypeAvailable(type)) {
        QSKIP("This GStreamer install lacks the stream type's encoder or muxer");
    }

    // For UDP the receiver listens and the server sends to it; servers listen on a free port.
    MockVideoStreamServer server;
    const bool receiverListens =
        (type == StreamType::RtpUdpH264) || (type == StreamType::RtpUdpH265) || (type == StreamType::MpegTsUdp);
    const quint16 port = receiverListens ? freeUdpPort() : 0;
    QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), port));

    _ignorePlayerLogs();
    ReceiverUnderTest stream(server.servedUri());
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.receiver->streaming());
    const VideoStreamStats stats = stream.receiver->stats();
    QCOMPARE(stats.decoder, QStringLiteral("Qt Multimedia"));
    QVERIFY(stats.fps > 0);
    QCOMPARE(stats.bitrateKbps, -1.0);
    QVERIFY(!stats.hasRtpStats());
    QVERIFY(stream.errorSpy.isEmpty());

    // Inactive: no frames, stats or state.
    stream.receiver->setActive(false);
    QCOMPARE(stream.receiver->state(), State::Stopped);
    QVERIFY(!stream.receiver->decoding());
    QVERIFY(!stream.receiver->stats().valid);
#endif
}

void QtMultimediaVideoReceiverTest::_testRestartReopensPort()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), freeUdpPort()));
    _ignorePlayerLogs();
    ReceiverUnderTest stream(server.servedUri(), true);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());

    // The new player binds the port once the old one let it go: no failed attempt, no reconnect. A restart counts
    // reconnects anew.
    stream.stateSpy.clear();
    stream.receiver->restart();
    QCOMPARE(stream.receiver->state(), State::Connecting);
    QVERIFY(stream.waitForDisplay());
    QVERIFY(!stream.stateSpy.contains(QVariantList{QVariant::fromValue(State::Reconnecting)}));
    QCOMPARE(stream.receiver->stats().reconnects, 0);
    QVERIFY(stream.errorSpy.isEmpty());
#endif
}

void QtMultimediaVideoReceiverTest::_testReconnectsAfterStreamLoss()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    const quint16 port = freeUdpPort();
    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    _ignorePlayerLogs();
    ReceiverUnderTest stream(server.servedUri(), true);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());

    // No frame for the timeout is a failure; it reconnects without telling the user.
    server.stop();
    QVERIFY(stream.waitForState(State::Reconnecting));
    QVERIFY(!stream.receiver->decoding());
    QVERIFY(!stream.receiver->stats().valid);

    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->stats().reconnects, 1);
    QVERIFY(stream.errorSpy.isEmpty());
#endif
}

void QtMultimediaVideoReceiverTest::_testStopsWithoutAutoReconnect()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), freeUdpPort()));
    _ignorePlayerLogs();
    ReceiverUnderTest stream(server.servedUri());
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());

    server.stop();
    QVERIFY(stream.waitForState(State::Stopped));
    QCOMPARE(stream.errorSpy.count(), 1);
    QVERIFY(!stream.stateSpy.contains(QVariantList{QVariant::fromValue(State::Reconnecting)}));
#endif
}

void QtMultimediaVideoReceiverTest::_testDetachingOutputDoesNotBlock()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    const quint16 port = freeUdpPort();
    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    _ignorePlayerLogs();
    constexpr auto kTimeout = std::chrono::seconds(10);
    ReceiverUnderTest stream(server.servedUri(), true, kTimeout);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());

    // The stream stalls, so the player waits on a read for up to the timeout: taking its output doesn't wait it out.
    server.stop();
    QElapsedTimer sinceDetached;
    sinceDetached.start();
    stream.receiver->setVideoOutput(nullptr);
    QTimer eventLoopTurn;
    eventLoopTurn.setSingleShot(true);
    QSignalSpy turned(&eventLoopTurn, &QTimer::timeout);
    eventLoopTurn.start(TestTimeout::shortDuration() / 4);
    QVERIFY(turned.wait(TestTimeout::longMs()));
    QVERIFY2(sinceDetached.elapsed() < (TestTimeout::shortDuration().count() * 2),
             qPrintable(QStringLiteral("blocked for %1 ms").arg(sinceDetached.elapsed())));
    QCOMPARE(stream.receiver->state(), State::Connecting);
    QVERIFY(!stream.receiver->decoding());

    // The session started without an output takes the next one.
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    stream.receiver->setVideoOutput(stream.output.get());
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->stats().reconnects, 0);
    QVERIFY(stream.errorSpy.isEmpty());
#endif
}

void QtMultimediaVideoReceiverTest::_testPlaysRtspOverTcp()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtspH264)) {
        QSKIP("This GStreamer install lacks the RTSP server or an H.264 encoder");
    }

    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtspH264, QStringLiteral("127.0.0.1"), 0));
    QUrl uri(server.servedUri());
    uri.setScheme(QStringLiteral("rtspt"));
    _ignorePlayerLogs();
    ReceiverUnderTest stream(uri.toString());
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(qgetenv("QT_FFMPEG_RTSP_TRANSPORT"), QByteArrayLiteral("tcp"));

    // A plain RTSP stream leaves the transport to FFmpeg again.
    VideoReceiver::Config config = stream.receiver->config();
    config.uri = server.servedUri();
    stream.receiver->setConfig(config);
    QVERIFY(stream.waitForDisplay());
    QVERIFY(qgetenv("QT_FFMPEG_RTSP_TRANSPORT").isNull());
    QVERIFY(stream.errorSpy.isEmpty());
#endif
}

void QtMultimediaVideoReceiverTest::_testRecordsStream()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), freeUdpPort()));
    _ignorePlayerLogs();
    _ignoreEncoderFallbacks();
    ReceiverUnderTest stream(server.servedUri());
    QSignalSpy startedSpy(stream.receiver.get(), &VideoReceiver::recordingStarted);
    QSignalSpy finishedSpy(stream.receiver.get(), &VideoReceiver::recordingFinished);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());

    // The frames on display, encoded anew.
    stream.receiver->startRecording(dir.filePath(QStringLiteral("stream.mkv")), VideoReceiver::FileFormat::Mkv);
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, TestTimeout::longMs());
    QVERIFY(stream.receiver->recording());
    const QString file = startedSpy.first().first().toString();
    QElapsedTimer recorded;
    recorded.start();
    QTRY_VERIFY_WITH_TIMEOUT(recorded.hasExpired(1500), TestTimeout::longMs());
    stream.receiver->stopRecording();
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::longMs());
    QCOMPARE(finishedSpy.first().at(0).toString(), file);
    QVERIFY2(finishedSpy.first().at(1).toString().isEmpty(), qPrintable(finishedSpy.first().at(1).toString()));
    QVERIFY(!stream.receiver->recording());

    QMediaPlayer player;
    player.setSource(QUrl::fromLocalFile(file));
    QTRY_VERIFY_WITH_TIMEOUT(player.mediaStatus() == QMediaPlayer::LoadedMedia, TestTimeout::mediumMs());
    QVERIFY2(player.duration() >= 1000, qPrintable(QString::number(player.duration())));
    QCOMPARE(player.metaData().value(QMediaMetaData::Resolution).toSize(), stream.receiver->videoSize());

    // Stopping the stream ends a recording.
    stream.receiver->startRecording(dir.filePath(QStringLiteral("stopped.mkv")), VideoReceiver::FileFormat::Mkv);
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 2, TestTimeout::longMs());
    stream.receiver->setActive(false);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 2, TestTimeout::longMs());
    QVERIFY(!stream.receiver->recording());
    QVERIFY(stream.errorSpy.isEmpty());
#endif
}
#endif

UT_REGISTER_TEST(QtMultimediaVideoReceiverTest, TestLabel::Integration)
