#include "VideoStreamResolverTest.h"

#include "VideoSettings.h"
#include "VideoStreamResolver.h"
#include "VideoTestHelpers.h"

using Role = VideoStreamResolver::Role;

namespace {

constexpr int kNoStreamInfo = -1;
constexpr uint32_t kRtspTimeoutSec = 12;

VideoStreamResolver::Settings manualSettings(const QString& videoSource)
{
    VideoStreamResolver::Settings settings;
    settings.videoSource = videoSource;
    settings.udpUrl = QStringLiteral("0.0.0.0:5600");
    settings.tcpUrl = QStringLiteral("192.168.1.2:5000");
    settings.rtspUrl = QStringLiteral("rtsp://user:pass@cam.local:554/manual");
    settings.rtspTimeoutSec = kRtspTimeoutSec;
    return settings;
}

}  // namespace

void VideoStreamResolverTest::_testResolve_data()
{
    QTest::addColumn<QString>("videoSource");
    QTest::addColumn<QString>("address");  ///< Overrides the source's URL setting when not null
    QTest::addColumn<int>("streamType");
    QTest::addColumn<int>("encoding");
    QTest::addColumn<QString>("streamUri");
    QTest::addColumn<int>("role");
    QTest::addColumn<QString>("expectedUri");  ///< Empty: no stream
    QTest::addColumn<int>("expectedTimeoutSec");

    const QString udp264 = VideoSettings::videoSourceUDPH264;
    const QString rtsp = VideoSettings::videoSourceRTSP;
    const int primary = static_cast<int>(Role::Primary);
    const int thermal = static_cast<int>(Role::Thermal);
    const int h264 = VIDEO_STREAM_ENCODING_H264;
    const int h265 = VIDEO_STREAM_ENCODING_H265;
    const int timeout = 3;
    const int rtspTimeout = kRtspTimeoutSec;

    // clang-format off
    QTest::newRow("disabled") << QString(VideoSettings::videoDisabled) << QString() << kNoStreamInfo << h264 << QString() << primary << QString() << 0;
    QTest::newRow("no video") << QString(VideoSettings::videoSourceNoVideo) << QString() << kNoStreamInfo << h264 << QString() << primary << QString() << 0;
    QTest::newRow("UVC camera") << QStringLiteral("Integrated Camera") << QString() << kNoStreamInfo << h264 << QString() << primary << QString() << 0;
    QTest::newRow("UDP h.264") << udp264 << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("udp://0.0.0.0:5600") << timeout;
    QTest::newRow("UDP h.264 without address") << udp264 << QStringLiteral("") << kNoStreamInfo << h264 << QString() << primary << QString() << 0;
    QTest::newRow("UDP h.265") << QString(VideoSettings::videoSourceUDPH265) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("udp265://0.0.0.0:5600") << timeout;
    QTest::newRow("MPEG-TS") << QString(VideoSettings::videoSourceMPEGTS) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("mpegts://0.0.0.0:5600") << timeout;
    QTest::newRow("RTSP") << rtsp << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("rtsp://user:pass@cam.local:554/manual") << rtspTimeout;
    QTest::newRow("RTSP without URL") << rtsp << QStringLiteral("") << kNoStreamInfo << h264 << QString() << primary << QString() << 0;
    QTest::newRow("TCP") << QString(VideoSettings::videoSourceTCP) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("tcp://192.168.1.2:5000") << timeout;
    QTest::newRow("3DR Solo") << QString(VideoSettings::videoSource3DRSolo) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("udp://0.0.0.0:5600") << timeout;
    QTest::newRow("Parrot Discovery") << QString(VideoSettings::videoSourceParrotDiscovery) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("udp://0.0.0.0:8888") << timeout;
    QTest::newRow("Yuneec Mantis G") << QString(VideoSettings::videoSourceYuneecMantisG) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("rtsp://192.168.42.1:554/live") << rtspTimeout;
    QTest::newRow("Herelink AirUnit") << QString(VideoSettings::videoSourceHerelinkAirUnit) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("rtsp://192.168.0.10:8554/H264Video") << rtspTimeout;
    QTest::newRow("Herelink Hotspot") << QString(VideoSettings::videoSourceHerelinkHotspot) << QString() << kNoStreamInfo << h264 << QString() << primary << QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream") << rtspTimeout;
    QTest::newRow("thermal ignores the manual source") << rtsp << QString() << kNoStreamInfo << h264 << QString() << thermal << QString() << 0;

    // A camera-advertised stream wins over the manual source, including a manual UDP address.
    QTest::newRow("auto RTSP over manual UDP") << udp264 << QString() << int(VIDEO_STREAM_TYPE_RTSP) << h264 << QStringLiteral("rtsp://10.0.0.1/main") << primary << QStringLiteral("rtsp://10.0.0.1/main") << rtspTimeout;
    QTest::newRow("auto UDP port over manual UDP") << udp264 << QString() << int(VIDEO_STREAM_TYPE_RTPUDP) << h264 << QStringLiteral("5601") << primary << QStringLiteral("udp://0.0.0.0:5601") << timeout;
    QTest::newRow("auto UDP h.265 port") << QString(VideoSettings::videoDisabled) << QString() << int(VIDEO_STREAM_TYPE_RTPUDP) << h265 << QStringLiteral("5602") << primary << QStringLiteral("udp265://0.0.0.0:5602") << timeout;
    QTest::newRow("auto UDP URI") << udp264 << QString() << int(VIDEO_STREAM_TYPE_RTPUDP) << h264 << QStringLiteral("udp://1.2.3.4:5600") << primary << QStringLiteral("udp://1.2.3.4:5600") << timeout;
    QTest::newRow("auto MPEG-TS port") << udp264 << QString() << int(VIDEO_STREAM_TYPE_MPEG_TS) << h264 << QStringLiteral("5603") << primary << QStringLiteral("mpegts://0.0.0.0:5603") << timeout;
    QTest::newRow("auto TCP over manual TCP") << QString(VideoSettings::videoSourceTCP) << QString() << int(VIDEO_STREAM_TYPE_TCP_MPEG) << h264 << QStringLiteral("tcp://10.0.0.1:5000") << primary << QStringLiteral("tcp://10.0.0.1:5000") << timeout;
    QTest::newRow("auto without URI falls back to manual") << udp264 << QString() << int(VIDEO_STREAM_TYPE_RTSP) << h264 << QString() << primary << QStringLiteral("udp://0.0.0.0:5600") << timeout;
    QTest::newRow("auto thermal RTSP") << rtsp << QString() << int(VIDEO_STREAM_TYPE_RTSP) << h264 << QStringLiteral("rtsp://10.0.0.1/thermal") << thermal << QStringLiteral("rtsp://10.0.0.1/thermal") << rtspTimeout;
    QTest::newRow("auto thermal UDP port") << rtsp << QString() << int(VIDEO_STREAM_TYPE_RTPUDP) << h264 << QStringLiteral("5610") << thermal << QStringLiteral("udp://0.0.0.0:5610") << timeout;
    QTest::newRow("auto thermal without URI") << rtsp << QString() << int(VIDEO_STREAM_TYPE_RTSP) << h264 << QString() << thermal << QString() << 0;
    // clang-format on
}

void VideoStreamResolverTest::_testResolve()
{
    QFETCH(QString, videoSource);
    QFETCH(QString, address);
    QFETCH(int, streamType);
    QFETCH(int, encoding);
    QFETCH(QString, streamUri);
    QFETCH(int, role);
    QFETCH(QString, expectedUri);
    QFETCH(int, expectedTimeoutSec);

    VideoStreamResolver::Settings settings = manualSettings(videoSource);
    if (!address.isNull()) {
        settings.udpUrl = address;
        settings.tcpUrl = address;
        settings.rtspUrl = address;
    }

    std::unique_ptr<QGCVideoStreamInfo> streamInfo;
    if (streamType != kNoStreamInfo) {
        streamInfo =
            VideoTestHelpers::makeStreamInfo(static_cast<VIDEO_STREAM_TYPE>(streamType), streamUri.toUtf8().constData(),
                                             static_cast<VIDEO_STREAM_ENCODING>(encoding));
    }

    const std::optional<VideoReceiver::Config> config =
        VideoStreamResolver::resolve(settings, streamInfo.get(), static_cast<Role>(role));
    if (expectedUri.isEmpty()) {
        QVERIFY(!config.has_value());
        return;
    }
    QVERIFY(config.has_value());
    QCOMPARE(config->uri, expectedUri);
    QCOMPARE(config->timeout.count(), expectedTimeoutSec);
}

void VideoStreamResolverTest::_testStreamSettingsPassThrough()
{
    VideoStreamResolver::Settings settings = manualSettings(VideoSettings::videoSourceUDPH264);
    settings.lowLatency = true;
    settings.rtpJitterLatencyMs = 250;
    settings.autoReconnect = false;
    const auto streamInfo = VideoTestHelpers::makeStreamInfo(VIDEO_STREAM_TYPE_RTSP, "rtsp://10.0.0.1/thermal");

    for (const auto& [info, role] : {std::pair{static_cast<QGCVideoStreamInfo*>(nullptr), Role::Primary},
                                     std::pair{streamInfo.get(), Role::Thermal}}) {
        const std::optional<VideoReceiver::Config> config = VideoStreamResolver::resolve(settings, info, role);
        QVERIFY(config.has_value());
        QVERIFY(config->lowLatency);
        QCOMPARE(config->rtpJitterLatencyMs, 250);
        QVERIFY(!config->autoReconnect);
    }
}

void VideoStreamResolverTest::_testNetworkSources_data()
{
    QTest::addColumn<QString>("videoSource");
    QTest::addColumn<bool>("networkSource");

    QTest::newRow("disabled") << QString(VideoSettings::videoDisabled) << false;
    QTest::newRow("no video") << QString(VideoSettings::videoSourceNoVideo) << false;
    QTest::newRow("UVC camera") << QStringLiteral("Integrated Camera") << false;
    QTest::newRow("RTSP") << QString(VideoSettings::videoSourceRTSP) << true;
    QTest::newRow("Herelink Hotspot") << QString(VideoSettings::videoSourceHerelinkHotspot) << true;
}

void VideoStreamResolverTest::_testNetworkSources()
{
    QFETCH(QString, videoSource);
    QFETCH(bool, networkSource);

    QCOMPARE(VideoStreamResolver::isNetworkSource(videoSource), networkSource);
}

UT_REGISTER_TEST(VideoStreamResolverTest, TestLabel::Unit)
