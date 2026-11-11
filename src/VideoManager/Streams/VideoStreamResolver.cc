#include "VideoStreamResolver.h"

#include <algorithm>
#include <climits>

#include <QtCore/QUrl>

#include "Fact.h"
#include "QGCVideoStreamInfo.h"
#include "VideoDecoderOptions.h"
#include "VideoSettings.h"
#include "VideoStreamUri.h"

namespace {

QString addressWithScheme(QLatin1StringView scheme, const QString& address)
{
    return address.isEmpty() ? QString() : (scheme + address);
}

/// The URI, of @p scheme, for a UDP or MPEG-TS stream a camera advertises as the port to listen on or as a URI, which
/// names the stream udp:// or by one of QGC's own schemes. A value that is neither is returned as it is, for the
/// receiver to turn down and report.
QString listenUri(QLatin1StringView scheme, const QString& advertised)
{
    bool isNumber = false;
    const int port = advertised.toInt(&isNumber);
    if (isNumber) {
        return VideoStreamUri::isValidPort(port) ? QStringLiteral("%1://0.0.0.0:%2").arg(scheme).arg(port) : advertised;
    }

    QUrl url(advertised);
    const std::optional<VideoStreamUri::Transport> transport = VideoStreamUri::transportFor(url);
    if (!transport || !VideoStreamUri::listens(*transport) || !VideoStreamUri::isValidPort(url.port())) {
        return advertised;
    }
    url.setScheme(scheme);
    if (url.host().isEmpty()) {
        url.setHost(QStringLiteral("0.0.0.0"));
    }
    return url.toString();
}

/// QGC's scheme for an RTP stream over UDP. The encoding is an extension field that a camera with an older dialect
/// doesn't send: mavlink-camera-manager names an H.265 stream udp265:// instead.
QLatin1StringView rtpUdpScheme(const QGCVideoStreamInfo& streamInfo)
{
    const bool namedH265 = (streamInfo.encoding() == VIDEO_STREAM_ENCODING_UNKNOWN) &&
                           streamInfo.uri().startsWith(QLatin1StringView("udp265://"), Qt::CaseInsensitive);
    return ((streamInfo.encoding() == VIDEO_STREAM_ENCODING_H265) || namedH265) ? QLatin1StringView("udp265")
                                                                                : QLatin1StringView("udp");
}

QString autoStreamUri(const QGCVideoStreamInfo& streamInfo)
{
    const QString uri = streamInfo.uri();
    switch (streamInfo.type()) {
        case VIDEO_STREAM_TYPE_RTPUDP:
            return listenUri(rtpUdpScheme(streamInfo), uri);
        case VIDEO_STREAM_TYPE_MPEG_TS:
            return listenUri(QLatin1StringView("mpegts"), uri);
        case VIDEO_STREAM_TYPE_RTSP:
        case VIDEO_STREAM_TYPE_TCP_MPEG:
        default:
            return uri;
    }
}

/// A manual video source a VideoReceiver plays.
struct ManualSource
{
    const char* videoSource;
    /// Prefixed to the source's URL setting, or the whole URI when the source has none.
    const char* uri;
    QString VideoStreamResolver::Settings::* url;
};

constexpr ManualSource kManualSources[] = {
    {VideoSettings::videoSourceUDPH264, "udp://", &VideoStreamResolver::Settings::udpUrl},
    {VideoSettings::videoSourceUDPH265, "udp265://", &VideoStreamResolver::Settings::udpUrl},
    {VideoSettings::videoSourceMPEGTS, "mpegts://", &VideoStreamResolver::Settings::udpUrl},
    {VideoSettings::videoSourceRTSP, "", &VideoStreamResolver::Settings::rtspUrl},
    {VideoSettings::videoSourceTCP, "tcp://", &VideoStreamResolver::Settings::tcpUrl},
    {VideoSettings::videoSource3DRSolo, "udp://0.0.0.0:5600", nullptr},
    {VideoSettings::videoSourceParrotDiscovery, "udp://0.0.0.0:8888", nullptr},
    {VideoSettings::videoSourceYuneecMantisG, "rtsp://192.168.42.1:554/live", nullptr},
    {VideoSettings::videoSourceHerelinkAirUnit, "rtsp://192.168.0.10:8554/H264Video", nullptr},
    {VideoSettings::videoSourceHerelinkHotspot, "rtsp://192.168.43.1:8554/fpv_stream", nullptr},
};

const ManualSource* findManualSource(const QString& videoSource)
{
    for (const ManualSource& source : kManualSources) {
        if (videoSource == QLatin1StringView(source.videoSource)) {
            return &source;
        }
    }
    return nullptr;
}

QString manualUri(const VideoStreamResolver::Settings& settings)
{
    const ManualSource* const source = findManualSource(settings.videoSource);
    if (!source) {
        return QString();
    }
    const QLatin1StringView uri(source->uri);
    return source->url ? addressWithScheme(uri, settings.*(source->url)) : QString(uri);
}

}  // namespace

VideoStreamResolver::Settings VideoStreamResolver::snapshot(VideoSettings* videoSettings)
{
    Settings settings;
    settings.videoSource = videoSettings->videoSource()->rawValue().toString();
    settings.udpUrl = videoSettings->udpUrl()->rawValue().toString();
    settings.tcpUrl = videoSettings->tcpUrl()->rawValue().toString();
    settings.rtspUrl = videoSettings->rtspUrl()->rawValue().toString();
    settings.rtspTimeoutSec = videoSettings->rtspTimeout()->rawValue().toUInt();
    settings.lowLatency = videoSettings->lowLatencyMode()->rawValue().toBool();
    settings.rtpJitterLatencyMs = static_cast<int>(
        std::min(videoSettings->rtpJitterLatencyMs()->rawValue().toUInt(), static_cast<uint>(INT_MAX)));
    settings.autoReconnect = videoSettings->rtspAutoReconnect()->rawValue().toBool();
    settings.conversionElement = videoSettings->videoConversionElement()->rawValue().toString().toUtf8();
    settings.disablePixelAspectRatio = videoSettings->disablePixelAspectRatio()->rawValue().toBool();
    settings.forceCpuVideoPath = videoSettings->forceCpuVideoPath()->rawValue().toBool();
    settings.forceVideoDecoder = videoDecoderOptionFromSetting(videoSettings->forceVideoDecoder()->rawValue().toInt());
    return settings;
}

VideoReceiver::DisplayConfig VideoStreamResolver::displayConfig(const Settings& settings)
{
    return {
        .conversionElement = settings.conversionElement,
        .disablePixelAspectRatio = settings.disablePixelAspectRatio,
        .gpuZeroCopy = !settings.forceCpuVideoPath && (settings.forceVideoDecoder != VideoDecoderOption::Software),
    };
}

std::optional<VideoReceiver::Config> VideoStreamResolver::resolve(const Settings& settings,
                                                                  const QGCVideoStreamInfo* streamInfo, Role role)
{
    QString uri;
    if (isAutoStream(streamInfo)) {
        uri = autoStreamUri(*streamInfo);
    } else if (role == Role::Primary) {
        uri = manualUri(settings);
    }
    if (uri.isEmpty()) {
        return std::nullopt;
    }

    VideoReceiver::Config config;
    config.uri = uri;
    // Only RTSP's stall timeout is a setting: the other streams keep the receiver's default.
    if (VideoStreamUri::transportFor(QUrl(uri)) == VideoStreamUri::Transport::Rtsp) {
        config.timeout = std::chrono::seconds(settings.rtspTimeoutSec);
    }
    config.lowLatency = settings.lowLatency;
    config.rtpJitterLatencyMs = settings.rtpJitterLatencyMs;
    config.autoReconnect = settings.autoReconnect;
    return config;
}

bool VideoStreamResolver::isAutoStream(const QGCVideoStreamInfo* streamInfo)
{
    return streamInfo && !streamInfo->uri().isEmpty();
}

bool VideoStreamResolver::isNetworkSource(const QString& videoSource)
{
    return findManualSource(videoSource) != nullptr;
}

QString VideoStreamResolver::transportName(const QString& uri)
{
    using VideoStreamUri::Transport;

    const QUrl url(uri);
    const std::optional<Transport> transport = VideoStreamUri::transportFor(url);
    if (!transport) {
        return url.scheme().toUpper();
    }
    switch (*transport) {
        case Transport::Rtsp:
            return QStringLiteral("RTSP");
        case Transport::RtpUdpH264:
            return QStringLiteral("RTP/UDP (H.264)");
        case Transport::RtpUdpH265:
            return QStringLiteral("RTP/UDP (H.265)");
        case Transport::MpegTsUdp:
            return QStringLiteral("MPEG-TS/UDP");
        case Transport::MpegTsTcp:
            return QStringLiteral("MPEG-TS/TCP");
    }
    return QString();
}
