#include "QtMultimediaVideoReceiverStreamTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QMediaPlayer>
#include <QtTest/QSignalSpy>

#include "MockVideoStreamServer.h"
#include "QtMultimediaReceiverTestSupport.h"
#include "QtMultimediaVideoReceiver.h"
#include "VideoTestHelpers.h"

using QtMultimediaTest::ReceiverUnderTest;
using VideoTestHelpers::freeUdpPort;

void QtMultimediaVideoReceiverStreamTest::_ignorePlayerLogs()
{
    for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::playerLogs()) {
        ignoreLogMessage(log.category, log.type, log.pattern);
    }
}

void QtMultimediaVideoReceiverStreamTest::_ignoreEncoderFallbacks()
{
    for (const QtMultimediaTest::IgnoredLog& log : QtMultimediaTest::encoderFallbackLogs()) {
        ignoreLogMessage(log.category, log.type, log.pattern);
    }
}

void QtMultimediaVideoReceiverStreamTest::_ignoreRtspServerLogs()
{
    // gst-rtsp-server defect, seen once with GStreamer 1.28.8 on a loaded macOS host: a client's socket is closed
    // while a main context of the server still polls it. GLib polls with select() there, which fails for a closed
    // descriptor where Linux's poll() only flags it, and GLib warns of the failure.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("poll\\(2\\) failed due to: Bad file descriptor")));
}

void QtMultimediaVideoReceiverStreamTest::_testPlaysMockStreams_data()
{
    QTest::addColumn<int>("streamType");

    using StreamType = MockVideoStreamServer::StreamType;
    QTest::newRow("rtp h264 udp") << int(StreamType::RtpUdpH264);
    QTest::newRow("rtp h265 udp") << int(StreamType::RtpUdpH265);
    QTest::newRow("mpeg-ts udp") << int(StreamType::MpegTsUdp);
    QTest::newRow("mpeg-ts tcp") << int(StreamType::MpegTsTcp);
    QTest::newRow("rtsp") << int(StreamType::RtspH264);
}

void QtMultimediaVideoReceiverStreamTest::_testPlaysMockStreams()
{
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
    _ignoreRtspServerLogs();
    ReceiverUnderTest stream(server.servedUri());
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Streaming);
    QVERIFY(stream.receiver->streaming());
    const VideoStreamStats stats = stream.receiver->stats();
    QCOMPARE(stats.decoder, QStringLiteral("Qt Multimedia"));
    QVERIFY(stats.fps > 0);
    QCOMPARE(stats.bitrateKbps, -1.0);
    QVERIFY(!stats.hasRtpStats());
    QVERIFY(stream.errorSpy.isEmpty());

    // Inactive: no frames, stats or state.
    stream.receiver->setActive(false);
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Stopped);
    QVERIFY(!stream.receiver->decoding());
    QVERIFY(!stream.receiver->stats().valid);
}

void QtMultimediaVideoReceiverStreamTest::_testDetachingOutputDoesNotBlock()
{
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
    QCOMPARE(stream.receiver->state(), VideoReceiver::State::Connecting);
    QVERIFY(!stream.receiver->decoding());

    // The session started without an output takes the next one.
    QVERIFY(server.start(StreamType::RtpUdpH264, QStringLiteral("127.0.0.1"), port));
    stream.receiver->setVideoOutput(stream.output.get());
    QVERIFY(stream.waitForDisplay());
    QCOMPARE(stream.receiver->stats().reconnects, 0);
    QVERIFY(stream.errorSpy.isEmpty());
}

void QtMultimediaVideoReceiverStreamTest::_testPlaysRtspOverTcp()
{
    using StreamType = MockVideoStreamServer::StreamType;
    if (!MockVideoStreamServer::isStreamTypeAvailable(StreamType::RtspH264)) {
        QSKIP("This GStreamer install lacks the RTSP server or an H.264 encoder");
    }

    MockVideoStreamServer server;
    QVERIFY(server.start(StreamType::RtspH264, QStringLiteral("127.0.0.1"), 0));
    QUrl uri(server.servedUri());
    uri.setScheme(QStringLiteral("rtspt"));
    _ignorePlayerLogs();
    _ignoreRtspServerLogs();
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
}

void QtMultimediaVideoReceiverStreamTest::_testRecordsStream()
{
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
    QVERIFY(stream.errorSpy.isEmpty());
}

UT_REGISTER_TEST(QtMultimediaVideoReceiverStreamTest, TestLabel::Integration)
