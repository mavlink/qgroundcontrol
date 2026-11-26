#include "FFmpegVideoReceiverStreamTest.h"

#include <algorithm>
#include <chrono>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QUrl>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "FFmpegPointers.h"
#include "FFmpegStreamSession.h"
#include "FFmpegVideoReceiver.h"
#include "MockVideoStreamServer.h"
#include "SettingsManager.h"
#include "VideoBackend.h"
#include "VideoSettings.h"
#include "VideoStreamStats.h"
#include "VideoTestHelpers.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using VideoTestHelpers::freeUdpPort;

namespace {

/// What libavformat reads of a recording's video.
struct RecordedVideo
{
    bool readable = false;
    AVCodecID codec = AV_CODEC_ID_NONE;
    QSize size;
    int frames = 0;
    /// From the first frame's time to the last's.
    qint64 durationMs = 0;
};

RecordedVideo readRecording(const QString& file)
{
    RecordedVideo video;
    AVFormatContext* context = nullptr;
    if (avformat_open_input(&context, QFile::encodeName(file).constData(), nullptr, nullptr) < 0) {
        return video;
    }
    const FFmpegPointers::Input input(context);
    if (avformat_find_stream_info(input.get(), nullptr) < 0) {
        return video;
    }
    const int index = av_find_best_stream(input.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0) {
        return video;
    }
    const AVCodecParameters* const parameters = input->streams[index]->codecpar;
    video.codec = parameters->codec_id;
    video.size = QSize(parameters->width, parameters->height);
    const AVRational timeBase = input->streams[index]->time_base;
    int64_t first = AV_NOPTS_VALUE;
    int64_t last = AV_NOPTS_VALUE;
    const FFmpegPointers::Packet packet(av_packet_alloc());
    while (av_read_frame(input.get(), packet.get()) >= 0) {
        if (packet->stream_index == index) {
            ++video.frames;
            if (packet->pts != AV_NOPTS_VALUE) {
                first = (first == AV_NOPTS_VALUE) ? packet->pts : std::min(first, packet->pts);
                last = (last == AV_NOPTS_VALUE) ? packet->pts : std::max(last, packet->pts);
            }
        }
        av_packet_unref(packet.get());
    }
    if (first != AV_NOPTS_VALUE) {
        video.durationMs = av_rescale_q(last - first, timeBase, AVRational{1, 1000});
    }
    video.readable = true;
    return video;
}

/// An FFmpegVideoReceiver playing @p uri into a VideoOutput, with spies on its signals.
struct FFmpegReceiverUnderTest
{
    explicit FFmpegReceiverUnderTest(const QString& uri, bool autoReconnect = false,
                                     std::chrono::seconds timeout = std::chrono::seconds(2),
                                     bool softwareDecoding = false)
        : output(VideoTestHelpers::createVideoOutput(engine))
        , receiver(std::make_unique<FFmpegVideoReceiver>())
        , stateSpy(receiver.get(), &VideoReceiver::stateChanged)
        , errorSpy(receiver.get(), &VideoReceiver::errorOccurred)
    {
        receiver->setName(QStringLiteral("test"));
        receiver->setConfig({.uri = uri, .timeout = timeout, .lowLatency = true, .autoReconnect = autoReconnect});
        VideoReceiver::DisplayConfig display;
        display.gpuZeroCopy = !softwareDecoding;
        receiver->setDisplayConfig(display);
        receiver->setVideoOutput(output.get());
    }

    [[nodiscard]] bool waitForState(VideoReceiver::State state,
                                    std::chrono::milliseconds timeout = TestTimeout::longDuration()) const
    {
        return UnitTest::waitForCondition([this, state] { return receiver->state() == state; }, timeout,
                                          u"receiver state");
    }

    /// Frames reach the output and the stats measure them: a sample this short can be one without a frame.
    [[nodiscard]] bool waitForDisplay() const
    {
        return UnitTest::waitForCondition(
            [this] {
                const VideoStreamStats stats = receiver->stats();
                return receiver->decoding() && receiver->videoSize().isValid() && stats.valid && (stats.fps > 0);
            },
            TestTimeout::longDuration(), u"display");
    }

    QQmlEngine engine;
    std::unique_ptr<QQuickItem> output;
    std::unique_ptr<FFmpegVideoReceiver> receiver;
    QSignalSpy stateSpy;
    QSignalSpy errorSpy;
};

}  // namespace

void FFmpegVideoReceiverStreamTest::_sampleOften(FFmpegVideoReceiver& receiver)
{
    receiver._sampleTimer.setInterval(std::chrono::milliseconds(200));
}

bool FFmpegVideoReceiverStreamTest::_sessionsEnded(const FFmpegVideoReceiver& receiver)
{
    return !receiver._session && receiver._endingSessions.empty();
}

void FFmpegVideoReceiverStreamTest::_ignoreRtspServerLogs()
{
    // As in QtMultimediaVideoReceiverStreamTest: a gst-rtsp-server defect, seen on a loaded macOS host.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("poll\\(2\\) failed due to: Bad file descriptor")));
}

void FFmpegVideoReceiverStreamTest::_testChosenByBackendSetting()
{
    Fact* const backend = SettingsManager::instance()->videoSettings()->videoBackend();
    QVERIFY(backend->enumValues().contains(QVariant(VideoSettings::VideoBackendFFmpeg)));
    VideoTestHelpers::FactRestorer facts;
    facts.set(backend, VideoSettings::VideoBackendFFmpeg);
    const std::unique_ptr<VideoReceiver> receiver(VideoBackend::instance().createReceiver(nullptr));
    QVERIFY(qobject_cast<FFmpegVideoReceiver*>(receiver.get()));
}

void FFmpegVideoReceiverStreamTest::_testSourceFor_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("sdpEncoding");
    QTest::addColumn<bool>("overTcp");

    QTest::newRow("rtsp") << QStringLiteral("rtsp://user:pw@10.0.0.2:8554/live") << true
                          << QStringLiteral("rtsp://user:pw@10.0.0.2:8554/live") << QString() << false;
    QTest::newRow("rtsp over tcp") << QStringLiteral("rtspt://10.0.0.2:8554/live") << true
                                   << QStringLiteral("rtsp://10.0.0.2:8554/live") << QString() << true;
    QTest::newRow("rtp h264") << QStringLiteral("udp://0.0.0.0:5600") << true << QStringLiteral("stream.sdp")
                              << QStringLiteral("H264") << false;
    QTest::newRow("rtp h265") << QStringLiteral("udp265://0.0.0.0:5600") << true << QStringLiteral("stream.sdp")
                              << QStringLiteral("H265") << false;
    QTest::newRow("mpeg-ts udp") << QStringLiteral("mpegts://0.0.0.0:5600") << true
                                 << QStringLiteral("udp://0.0.0.0:5600") << QString() << false;
    QTest::newRow("mpeg-ts tcp") << QStringLiteral("tcp://10.0.0.2:5600") << true
                                 << QStringLiteral("tcp://10.0.0.2:5600") << QString() << false;
    QTest::newRow("free port to listen on")
        << QStringLiteral("udp://0.0.0.0:0") << false << QString() << QString() << false;
    QTest::newRow("tcp without host") << QStringLiteral("tcp://:5600") << false << QString() << QString() << false;
    QTest::newRow("no scheme") << QStringLiteral("10.0.0.2:5600") << false << QString() << QString() << false;
}

void FFmpegVideoReceiverStreamTest::_testSourceFor()
{
    QFETCH(QString, uri);
    QFETCH(bool, valid);
    QFETCH(QString, url);
    QFETCH(QString, sdpEncoding);
    QFETCH(bool, overTcp);

    const std::optional<FFmpegStreamSession::Source> source = FFmpegStreamSession::sourceFor(uri);
    QCOMPARE(source.has_value(), valid);
    if (!source) {
        return;
    }
    QCOMPARE(source->url, url);
    QCOMPARE(source->rtspOverTcp, overTcp);
    if (sdpEncoding.isEmpty()) {
        QVERIFY(source->sdp.isEmpty());
    } else {
        QVERIFY2(source->sdp.contains("m=video 5600 RTP/AVP 96\r\n"), source->sdp.constData());
        QVERIFY2(source->sdp.contains(QStringLiteral("a=rtpmap:96 %1/90000\r\n").arg(sdpEncoding).toLatin1()),
                 source->sdp.constData());
    }
}

void FFmpegVideoReceiverStreamTest::_testPlaysMockStreams_data()
{
    QTest::addColumn<int>("streamType");
    QTest::addColumn<bool>("overTcp");
    QTest::addColumn<bool>("softwareDecoding");
    QTest::addColumn<QString>("codec");

    using StreamType = MockVideoStreamServer::StreamType;
    const QString h264 = QStringLiteral("H.264");
    QTest::newRow("rtp h264 udp") << int(StreamType::RtpUdpH264) << false << false << h264;
    QTest::newRow("rtp h264 udp, software decoder") << int(StreamType::RtpUdpH264) << false << true << h264;
    QTest::newRow("rtp h265 udp") << int(StreamType::RtpUdpH265) << false << false << QStringLiteral("H.265");
    QTest::newRow("mpeg-ts udp") << int(StreamType::MpegTsUdp) << false << false << h264;
    QTest::newRow("mpeg-ts tcp") << int(StreamType::MpegTsTcp) << false << false << h264;
    QTest::newRow("rtsp") << int(StreamType::RtspH264) << false << false << h264;
    QTest::newRow("rtsp over tcp") << int(StreamType::RtspH264) << true << false << h264;
}

void FFmpegVideoReceiverStreamTest::_testPlaysMockStreams()
{
    QFETCH(int, streamType);
    QFETCH(bool, overTcp);
    QFETCH(bool, softwareDecoding);
    QFETCH(QString, codec);

    using StreamType = MockVideoStreamServer::StreamType;
    const auto type = static_cast<StreamType>(streamType);
    if (!MockVideoStreamServer::isStreamTypeAvailable(type)) {
        QSKIP("This GStreamer install lacks the stream type's encoder or muxer");
    }

    // For UDP the receiver listens and the server sends to it; servers listen on a free port.
    MockVideoStreamServer server;
    const bool receiverListens =
        (type == StreamType::RtpUdpH264) || (type == StreamType::RtpUdpH265) || (type == StreamType::MpegTsUdp);
    QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), receiverListens ? freeUdpPort() : 0));
    QUrl uri(server.servedUri());
    if (overTcp) {
        uri.setScheme(QStringLiteral("rtspt"));
    }

    _ignoreRtspServerLogs();
    FFmpegReceiverUnderTest stream(uri.toString(), false, std::chrono::seconds(2), softwareDecoding);
    _sampleOften(*stream.receiver);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Streaming);
    QCOMPARE(stream.receiver->videoSize(), QSize(1280, 720));
    // The server sends a keyframe a second.
    QTRY_VERIFY_WITH_TIMEOUT(stream.receiver->stats().keyframeIntervalMs > 0, TestTimeout::longMs());
    const VideoStreamStats stats = stream.receiver->stats();
    QVERIFY2((stats.keyframeIntervalMs > 500) && (stats.keyframeIntervalMs < 2000),
             qPrintable(QString::number(stats.keyframeIntervalMs)));
    QCOMPARE(stats.codec, codec);
    QVERIFY(!stats.decoder.isEmpty());
    QVERIFY(stats.fps > 0);
    QVERIFY(stats.bitrateKbps > 0);
    QVERIFY(stats.uptimeMs > 0);
    QVERIFY(!stats.hasRtpStats());
    if (softwareDecoding) {
        QVERIFY(!stats.hardwareDecoder);
        QCOMPARE(stats.gpuPath, QStringLiteral("CPU"));
    }
    QVERIFY(stream.errorSpy.isEmpty());

    // Inactive: no frames, stats or state.
    stream.receiver->setActive(false);
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Stopped);
    QVERIFY(!stream.receiver->decoding());
    QVERIFY(!stream.receiver->stats().valid);
}

void FFmpegVideoReceiverStreamTest::_testStopWhileConnectingIsPrompt_data()
{
    QTest::addColumn<QString>("scheme");

    // A server that accepts the connection and never answers, and a UDP port nobody sends to.
    QTest::newRow("rtsp") << QStringLiteral("rtsp");
    QTest::newRow("mpeg-ts tcp") << QStringLiteral("tcp");
    QTest::newRow("rtp udp") << QStringLiteral("udp");
}

void FFmpegVideoReceiverStreamTest::_testStopWhileConnectingIsPrompt()
{
    QFETCH(QString, scheme);

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QSignalSpy connected(&server, &QTcpServer::newConnection);
    const bool listens = (scheme == QStringLiteral("udp"));
    const quint16 udpPort = listens ? freeUdpPort() : 0;
    const QString uri = listens ? QStringLiteral("udp://127.0.0.1:%1").arg(udpPort)
                                : QStringLiteral("%1://127.0.0.1:%2/test").arg(scheme).arg(server.serverPort());

    // FFmpeg would wait this long for an answer by itself.
    FFmpegReceiverUnderTest stream(uri, false, std::chrono::seconds(30));
    _sampleOften(*stream.receiver);
    stream.receiver->setActive(true);
    if (listens) {
        // The session listens once the port is taken.
        QVERIFY(UnitTest::waitForCondition(
            [udpPort] {
                QUdpSocket probe;
                return !probe.bind(QHostAddress::AnyIPv4, udpPort, QAbstractSocket::DontShareAddress);
            },
            TestTimeout::mediumDuration(), u"port bound"));
    } else {
        QVERIFY_SIGNAL_COUNT_WAIT(connected, 1, TestTimeout::mediumMs());
    }
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Connecting);

    QElapsedTimer stopping;
    stopping.start();
    stream.receiver->setActive(false);
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Stopped);
    // The session's thread, blocked in FFmpeg, ends as soon as FFmpeg asks the interrupt callback.
    QVERIFY(UnitTest::waitForCondition([&stream] { return _sessionsEnded(*stream.receiver); },
                                       TestTimeout::shortDuration(), u"session ended"));
    QVERIFY2(stopping.elapsed() < TestTimeout::shortMs(), qPrintable(QString::number(stopping.elapsed())));
    QVERIFY(stream.errorSpy.isEmpty());
}

void FFmpegVideoReceiverStreamTest::_testReconnectsAfterStreamLoss_data()
{
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("auto-reconnect") << true;
    QTest::newRow("no auto-reconnect") << false;
}

void FFmpegVideoReceiverStreamTest::_testReconnectsAfterStreamLoss()
{
    QFETCH(bool, autoReconnect);

    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    const quint16 port = freeUdpPort();
    MockVideoStreamServer server;
    FFmpegReceiverUnderTest stream(QStringLiteral("udp://127.0.0.1:%1").arg(port), autoReconnect,
                                   std::chrono::seconds(1));
    _sampleOften(*stream.receiver);
    stream.receiver->setActive(true);

    // Nobody sends yet: the port is listened on past the timeout, without a failure. Two timeouts and a half, which
    // TestTimeout doesn't scale to.
    QVERIFY_NO_SIGNAL_WAIT(stream.errorSpy, 2500);
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Connecting);
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    QVERIFY(stream.waitForDisplay());

    // The stream goes away: noticed after the timeout.
    server.stop();
    if (!autoReconnect) {
        QVERIFY(stream.waitForState(VideoReceiver::State::Stopped));
        QCOMPARE(stream.errorSpy.count(), 1);
        return;
    }
    QVERIFY(stream.waitForState(VideoReceiver::State::Reconnecting));
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Streaming);
    QCOMPARE(stream.receiver->stats().reconnects, 1);
    QVERIFY(stream.errorSpy.isEmpty());
}

void FFmpegVideoReceiverStreamTest::_testRecordsStream_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<bool>("withOutput");

    QTest::newRow("matroska") << int(VideoReceiver::FileFormat::Mkv) << true;
    QTest::newRow("mp4") << int(VideoReceiver::FileFormat::Mp4) << true;
    QTest::newRow("mov") << int(VideoReceiver::FileFormat::Mov) << true;
    QTest::newRow("mp4 without an output") << int(VideoReceiver::FileFormat::Mp4) << false;
}

void FFmpegVideoReceiverStreamTest::_testRecordsStream()
{
    QFETCH(int, format);
    QFETCH(bool, withOutput);

    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), freeUdpPort()));
    FFmpegReceiverUnderTest stream(server.servedUri());
    _sampleOften(*stream.receiver);
    if (!withOutput) {
        stream.receiver->setVideoOutput(nullptr);
    }
    QSignalSpy startedSpy(stream.receiver.get(), &VideoReceiver::recordingStarted);
    QSignalSpy finishedSpy(stream.receiver.get(), &VideoReceiver::recordingFinished);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(VideoReceiver::State::Streaming));

    const auto fileFormat = static_cast<VideoReceiver::FileFormat>(format);
    const QString file = dir.filePath(QStringLiteral("stream.%1").arg(VideoReceiver::fileExtension(fileFormat)));
    stream.receiver->startRecording(file, fileFormat);
    QVERIFY(stream.receiver->recording());
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, TestTimeout::longMs());
    QCOMPARE(startedSpy.first().first().toString(), file);
    // Two seconds of the stream, as the statistics tell.
    QTRY_VERIFY_WITH_TIMEOUT(
        (stream.receiver->stats().recordingMs > 2000) && (stream.receiver->stats().recordingBytes > 0),
        TestTimeout::longMs());
    const qint64 recordedMs = stream.receiver->stats().recordingMs;
    stream.receiver->stopRecording();
    QVERIFY(!stream.receiver->recording());
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::longMs());
    QCOMPARE(finishedSpy.first().at(0).toString(), file);
    QVERIFY2(finishedSpy.first().at(1).toString().isEmpty(), qPrintable(finishedSpy.first().at(1).toString()));
    QTRY_COMPARE_WITH_TIMEOUT(stream.receiver->stats().recordingMs, qint64(-1), TestTimeout::mediumMs());

    // Finished as recordingFinished() said: the whole file reads, packets copied as they arrived.
    const RecordedVideo video = readRecording(file);
    QVERIFY(video.readable);
    QCOMPARE(video.codec, AV_CODEC_ID_H264);
    QCOMPARE(video.size, QSize(1280, 720));
    QVERIFY2(video.frames >= 30, qPrintable(QString::number(video.frames)));
    // The stream's times, from the first keyframe on: no more than the recording took, and its 30 frames a second.
    QVERIFY2((video.durationMs >= 1000) && (video.durationMs <= recordedMs + 2000),
             qPrintable(QStringLiteral("%1 ms of %2").arg(video.durationMs).arg(recordedMs)));
    QVERIFY2(qAbs((video.frames * 1000.0 / 30.0) - video.durationMs) < 500,
             qPrintable(QStringLiteral("%1 frames in %2 ms").arg(video.frames).arg(video.durationMs)));
    // MP4 and MOV in fragments, which a crash leaves readable up to the last one.
    if (fileFormat != VideoReceiver::FileFormat::Mkv) {
        QFile recorded(file);
        QVERIFY(recorded.open(QIODevice::ReadOnly));
        QVERIFY(recorded.readAll().contains("moof"));
    }
    QVERIFY(stream.errorSpy.isEmpty());
}

void FFmpegVideoReceiverStreamTest::_testRecordingContinuesAfterReconnect()
{
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtpUdpH264)) {
        QSKIP("This GStreamer install lacks an H.264 encoder");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const quint16 port = freeUdpPort();
    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    FFmpegReceiverUnderTest stream(server.servedUri(), true, std::chrono::seconds(1));
    _sampleOften(*stream.receiver);
    QSignalSpy startedSpy(stream.receiver.get(), &VideoReceiver::recordingStarted);
    QSignalSpy finishedSpy(stream.receiver.get(), &VideoReceiver::recordingFinished);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());
    const QString file = dir.filePath(QStringLiteral("stream.mkv"));
    stream.receiver->startRecording(file, VideoReceiver::FileFormat::Mkv);
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, TestTimeout::longMs());

    // The stream is lost: its part finishes, and the recording goes on.
    server.stop();
    QVERIFY(stream.waitForState(VideoReceiver::State::Reconnecting));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, TestTimeout::longMs());
    QCOMPARE(finishedSpy.first().at(0).toString(), file);
    QVERIFY(stream.receiver->recording());

    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 2, TestTimeout::longMs());
    const QString secondPart = dir.filePath(QStringLiteral("stream_part2.mkv"));
    QCOMPARE(startedSpy.at(1).first().toString(), secondPart);
    stream.receiver->stopRecording();
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 2, TestTimeout::longMs());
    QCOMPARE(finishedSpy.at(1).at(0).toString(), secondPart);

    for (const QString& part : {file, secondPart}) {
        const RecordedVideo video = readRecording(part);
        QVERIFY2(video.readable && (video.frames > 0), qPrintable(part));
    }
    QVERIFY(stream.errorSpy.isEmpty());
}

UT_REGISTER_TEST(FFmpegVideoReceiverStreamTest, TestLabel::Integration)
