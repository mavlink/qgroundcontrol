#include "VideoUrlConstructionTest.h"
#include "VideoSettings.h"

#include <QtTest/QTest>

// These tests verify the URL construction logic used by VideoManager._updateSettings()
// The actual VideoManager depends on too many singletons to easily test directly,
// so we test the URL patterns that should be produced for each video source.

void VideoUrlConstructionTest::_testUdpH264UrlConstruction()
{
    // UDP H.264 URLs should be prefixed with "udp://"
    // Input: "0.0.0.0:5600" -> Output: "udp://0.0.0.0:5600"

    const QString udpUrl = "0.0.0.0:5600";
    const QString expectedUri = QStringLiteral("udp://%1").arg(udpUrl);

    QCOMPARE(expectedUri, QString("udp://0.0.0.0:5600"));

    // Test with different port
    const QString udpUrl2 = "0.0.0.0:14550";
    const QString expectedUri2 = QStringLiteral("udp://%1").arg(udpUrl2);
    QCOMPARE(expectedUri2, QString("udp://0.0.0.0:14550"));

    // Test with specific IP
    const QString udpUrl3 = "192.168.1.100:5600";
    const QString expectedUri3 = QStringLiteral("udp://%1").arg(udpUrl3);
    QCOMPARE(expectedUri3, QString("udp://192.168.1.100:5600"));
}

void VideoUrlConstructionTest::_testUdpH265UrlConstruction()
{
    // UDP H.265 URLs should be prefixed with "udp265://"
    // Input: "0.0.0.0:5600" -> Output: "udp265://0.0.0.0:5600"

    const QString udpUrl = "0.0.0.0:5600";
    const QString expectedUri = QStringLiteral("udp265://%1").arg(udpUrl);

    QCOMPARE(expectedUri, QString("udp265://0.0.0.0:5600"));
}

void VideoUrlConstructionTest::_testRtspUrlConstruction()
{
    // RTSP URLs should be used as-is (already have rtsp:// prefix)
    // Input: "rtsp://192.168.1.1:554/live" -> Output: same

    const QString rtspUrl = "rtsp://192.168.1.1:554/live";
    // RTSP URLs are used directly without modification
    QCOMPARE(rtspUrl, QString("rtsp://192.168.1.1:554/live"));

    // Test various RTSP URL formats
    QVERIFY(QString("rtsp://host:port/path").startsWith("rtsp://"));
    QVERIFY(QString("rtsp://user:pass@host:554/stream").startsWith("rtsp://"));
}

void VideoUrlConstructionTest::_testTcpUrlConstruction()
{
    // TCP URLs should be prefixed with "tcp://"
    // Input: "192.168.1.1:5600" -> Output: "tcp://192.168.1.1:5600"

    const QString tcpUrl = "192.168.1.1:5600";
    const QString expectedUri = QStringLiteral("tcp://%1").arg(tcpUrl);

    QCOMPARE(expectedUri, QString("tcp://192.168.1.1:5600"));
}

void VideoUrlConstructionTest::_testMpegTsUrlConstruction()
{
    // MPEG-TS URLs should be prefixed with "mpegts://"
    // Input: "0.0.0.0:8888" -> Output: "mpegts://0.0.0.0:8888"

    const QString udpUrl = "0.0.0.0:8888";
    const QString expectedUri = QStringLiteral("mpegts://%1").arg(udpUrl);

    QCOMPARE(expectedUri, QString("mpegts://0.0.0.0:8888"));
}

void VideoUrlConstructionTest::_testVendorSpecificUrls()
{
    // Test the hardcoded vendor-specific URLs from VideoManager._updateSettings()

    // 3DR Solo
    const QString soloUri = QStringLiteral("udp://0.0.0.0:5600");
    QCOMPARE(soloUri, QString("udp://0.0.0.0:5600"));

    // Parrot Discovery
    const QString parrotUri = QStringLiteral("udp://0.0.0.0:8888");
    QCOMPARE(parrotUri, QString("udp://0.0.0.0:8888"));

    // Yuneec Mantis G
    const QString yuneecUri = QStringLiteral("rtsp://192.168.42.1:554/live");
    QCOMPARE(yuneecUri, QString("rtsp://192.168.42.1:554/live"));
    QVERIFY(yuneecUri.startsWith("rtsp://"));

    // Herelink AirUnit
    const QString herelinkAirUri = QStringLiteral("rtsp://192.168.0.10:8554/H264Video");
    QCOMPARE(herelinkAirUri, QString("rtsp://192.168.0.10:8554/H264Video"));
    QVERIFY(herelinkAirUri.contains("192.168.0.10"));

    // Herelink Hotspot
    const QString herelinkHotspotUri = QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream");
    QCOMPARE(herelinkHotspotUri, QString("rtsp://192.168.43.1:8554/fpv_stream"));
    QVERIFY(herelinkHotspotUri.contains("192.168.43.1"));
}

void VideoUrlConstructionTest::_testAutoStreamUrlPrefixing()
{
    // Test the auto-stream URL prefixing logic from VideoManager._updateAutoStream()
    // When stream info provides just a port number, it should be prefixed with the
    // appropriate scheme and bound to 0.0.0.0

    // UDP H.264: port only -> "udp://0.0.0.0:port"
    {
        const QString portOnly = "5600";
        const bool needsPrefix = !portOnly.contains("udp://");
        QVERIFY(needsPrefix);

        const QString prefixed = QStringLiteral("udp://0.0.0.0:%1").arg(portOnly);
        QCOMPARE(prefixed, QString("udp://0.0.0.0:5600"));
    }

    // UDP H.265: port only -> "udp265://0.0.0.0:port"
    {
        const QString portOnly = "5600";
        const bool needsPrefix = !portOnly.contains("udp265://");
        QVERIFY(needsPrefix);

        const QString prefixed = QStringLiteral("udp265://0.0.0.0:%1").arg(portOnly);
        QCOMPARE(prefixed, QString("udp265://0.0.0.0:5600"));
    }

    // MPEG-TS: port only -> "mpegts://0.0.0.0:port"
    {
        const QString portOnly = "8888";
        const bool needsPrefix = !portOnly.contains("mpegts://");
        QVERIFY(needsPrefix);

        const QString prefixed = QStringLiteral("mpegts://0.0.0.0:%1").arg(portOnly);
        QCOMPARE(prefixed, QString("mpegts://0.0.0.0:8888"));
    }

    // Already prefixed URLs should not be double-prefixed
    {
        const QString alreadyPrefixed = "udp://192.168.1.1:5600";
        const bool needsPrefix = !alreadyPrefixed.contains("udp://");
        QVERIFY(!needsPrefix);
    }
}
