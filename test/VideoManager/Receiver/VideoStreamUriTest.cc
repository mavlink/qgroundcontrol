#include "VideoStreamUriTest.h"

#include "VideoStreamUri.h"

void VideoStreamUriTest::_testFFmpegSource_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("sdpEncoding");
    QTest::addColumn<bool>("rtspOverTcp");
    QTest::addColumn<bool>("streamsDescribed");

    QTest::newRow("rtsp") << QStringLiteral("rtsp://user:pw@192.168.42.1:554/live")
                          << QStringLiteral("rtsp://user:pw@192.168.42.1:554/live") << QString() << false << true;
    QTest::newRow("rtspt") << QStringLiteral("rtspt://192.168.42.1:554/live")
                           << QStringLiteral("rtsp://192.168.42.1:554/live") << QString() << true << true;
    QTest::newRow("rtsps") << QStringLiteral("rtsps://cam/live") << QStringLiteral("rtsps://cam/live") << QString()
                           << false << true;
    // MPEG-TS, over TCP or UDP, tells its streams only as it plays.
    QTest::newRow("tcp") << QStringLiteral("tcp://192.168.143.200:3001") << QStringLiteral("tcp://192.168.143.200:3001")
                         << QString() << false << false;
    QTest::newRow("mpegts") << QStringLiteral("mpegts://0.0.0.0:5600") << QStringLiteral("udp://0.0.0.0:5600")
                            << QString() << false << false;
    QTest::newRow("udp h264") << QStringLiteral("udp://0.0.0.0:5600") << QStringLiteral("stream.sdp")
                              << QStringLiteral("H264") << false << true;
    QTest::newRow("udp h265") << QStringLiteral("udp265://0.0.0.0:5601") << QStringLiteral("stream.sdp")
                              << QStringLiteral("H265") << false << true;
    QTest::newRow("none of the resolver's") << QStringLiteral("http://cam/live.m3u8")
                                            << QStringLiteral("http://cam/live.m3u8") << QString() << false << false;
}

void VideoStreamUriTest::_testFFmpegSource()
{
    QFETCH(QString, uri);
    QFETCH(QString, url);
    QFETCH(QString, sdpEncoding);
    QFETCH(bool, rtspOverTcp);
    QFETCH(bool, streamsDescribed);

    const std::optional<VideoStreamUri::FFmpegSource> source = VideoStreamUri::ffmpegSource(uri);
    QVERIFY(source);
    QCOMPARE(source->url, QUrl(url));
    QCOMPARE(source->transport, VideoStreamUri::transportFor(QUrl(uri)));
    QCOMPARE(source->rtspOverTcp, rtspOverTcp);
    QCOMPARE(source->streamsDescribed(), streamsDescribed);
    if (sdpEncoding.isEmpty()) {
        QVERIFY(source->sdp.isEmpty());
        return;
    }
    // RTP over UDP: an SDP description of payload type 96 at the URI's port.
    const QByteArray port = QByteArray::number(QUrl(uri).port());
    QVERIFY2(source->sdp.contains("m=video " + port + " RTP/AVP 96\r\n"), source->sdp.constData());
    QVERIFY2(source->sdp.contains("a=rtpmap:96 " + sdpEncoding.toLatin1() + "/90000\r\n"), source->sdp.constData());
    QVERIFY2(source->sdp.contains("c=IN IP4 0.0.0.0\r\n"), source->sdp.constData());
}

void VideoStreamUriTest::_testFFmpegSourceRefusesUnplayableUri_data()
{
    QTest::addColumn<QString>("uri");

    QTest::newRow("empty") << QString();
    QTest::newRow("no scheme") << QStringLiteral("0.0.0.0:5600");
    // Addresses FFmpeg would be given only to fail on at every reconnect.
    QTest::newRow("udp without port") << QStringLiteral("udp://0.0.0.0");
    QTest::newRow("free port to listen on") << QStringLiteral("udp://0.0.0.0:0");
    QTest::newRow("mpegts without port") << QStringLiteral("mpegts://0.0.0.0");
    QTest::newRow("tcp without port") << QStringLiteral("tcp://192.168.143.200");
    QTest::newRow("tcp without host") << QStringLiteral("tcp://:3001");
}

void VideoStreamUriTest::_testFFmpegSourceRefusesUnplayableUri()
{
    QFETCH(QString, uri);

    QVERIFY(!VideoStreamUri::ffmpegSource(uri));
}

UT_REGISTER_TEST(VideoStreamUriTest, TestLabel::Unit)
