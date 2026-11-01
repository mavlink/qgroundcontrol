#include "QtMultimediaVideoReceiverTest.h"

#include <memory>

#include <QtCore/QRandomGenerator>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
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
struct Receiver
{
    explicit Receiver(const QString& uri, bool autoReconnect = false,
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

void QtMultimediaVideoReceiverTest::_ignoreLoopWarning()
{
    ignoreLogMessage("default", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Cannot set loops for non-seekable source$")));
}

void QtMultimediaVideoReceiverTest::_testPlayerSource_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("sdpEncoding");

    QTest::newRow("rtsp") << QStringLiteral("rtsp://192.168.42.1:554/live")
                          << QStringLiteral("rtsp://192.168.42.1:554/live") << QString();
    QTest::newRow("rtsps") << QStringLiteral("rtsps://cam/live") << QStringLiteral("rtsps://cam/live") << QString();
    QTest::newRow("tcp") << QStringLiteral("tcp://192.168.143.200:3001") << QStringLiteral("tcp://192.168.143.200:3001")
                         << QString();
    QTest::newRow("mpegts") << QStringLiteral("mpegts://0.0.0.0:5600") << QStringLiteral("udp://0.0.0.0:5600")
                            << QString();
    QTest::newRow("udp h264") << QStringLiteral("udp://0.0.0.0:5600") << QStringLiteral("stream.sdp")
                              << QStringLiteral("H264");
    QTest::newRow("udp h265") << QStringLiteral("udp265://0.0.0.0:5601") << QStringLiteral("stream.sdp")
                              << QStringLiteral("H265");
}

void QtMultimediaVideoReceiverTest::_testPlayerSource()
{
    QFETCH(QString, uri);
    QFETCH(QString, url);
    QFETCH(QString, sdpEncoding);

    const std::optional<QtMultimediaVideoReceiver::Source> source = QtMultimediaVideoReceiver::playerSource(uri);
    QVERIFY(source);
    QCOMPARE(source->url, QUrl(url));
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

    Receiver stream(QStringLiteral("udp://0.0.0.0"));
    expectLogMessage("Video.QtMultimediaVideoReceiver", QtWarningMsg, QRegularExpression(QStringLiteral("can't play")));
    stream.receiver->setActive(true);
    verifyExpectedLogMessage();
    QCOMPARE(stream.receiver->state(), State::Stopped);
    QCOMPARE(stream.errorSpy.count(), 1);
}

void QtMultimediaVideoReceiverTest::_testRecordingUnsupported()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString file = dir.filePath(QStringLiteral("stream.mp4"));

    Receiver stream(QStringLiteral("udp://127.0.0.1:%1").arg(freeUdpPort()));
    stream.receiver->setActive(true);
    QCOMPARE(stream.receiver->state(), State::Connecting);

    expectLogMessage("Video.QtMultimediaVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Not recording")));
    stream.receiver->startRecording(file, VideoReceiver::FileFormat::Mp4);
    verifyExpectedLogMessage();
    QCOMPARE(stream.errorSpy.count(), 1);
    QVERIFY(!stream.receiver->recording());
    QVERIFY(!QFile::exists(file));

    stream.receiver->setActive(false);
    QCOMPARE(stream.receiver->state(), State::Stopped);
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

    _ignoreLoopWarning();
    Receiver stream(server.servedUri());
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
    _ignoreLoopWarning();
    Receiver stream(server.servedUri(), true);
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
    _ignoreLoopWarning();
    Receiver stream(server.servedUri(), true);
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
    _ignoreLoopWarning();
    Receiver stream(server.servedUri());
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());

    server.stop();
    QVERIFY(stream.waitForState(State::Stopped));
    QCOMPARE(stream.errorSpy.count(), 1);
    QVERIFY(!stream.stateSpy.contains(QVariantList{QVariant::fromValue(State::Reconnecting)}));
#endif
}

#endif

UT_REGISTER_TEST(QtMultimediaVideoReceiverTest, TestLabel::Integration)
