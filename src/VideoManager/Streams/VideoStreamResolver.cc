#include "VideoStreamResolver.h"

#include <algorithm>
#include <climits>

#include "Fact.h"
#include "QGCVideoStreamInfo.h"
#include "VideoDecoderOptions.h"
#include "VideoSettings.h"

namespace {

// Stall timeout of every stream but RTSP, whose timeout is a setting.
constexpr std::chrono::seconds kNonRtspStreamTimeout{3};

QString addressWithScheme(QLatin1StringView scheme, const QString& address)
{
    return address.isEmpty() ? QString() : (scheme + address);
}

/// A camera advertises a UDP or MPEG-TS stream as either a URI or just the port to listen on.
QString listenUriWithScheme(QLatin1StringView scheme, const QString& uriOrPort)
{
    return uriOrPort.contains(scheme) ? uriOrPort : (scheme + QStringLiteral("0.0.0.0:") + uriOrPort);
}

QString autoStreamUri(const QGCVideoStreamInfo& streamInfo)
{
    const QString uri = streamInfo.uri();
    switch (streamInfo.type()) {
        case VIDEO_STREAM_TYPE_RTPUDP:
            return (streamInfo.encoding() == VIDEO_STREAM_ENCODING_H265)
                       ? listenUriWithScheme(QLatin1StringView("udp265://"), uri)
                       : listenUriWithScheme(QLatin1StringView("udp://"), uri);
        case VIDEO_STREAM_TYPE_MPEG_TS:
            return listenUriWithScheme(QLatin1StringView("mpegts://"), uri);
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
    settings.forceVideoDecoder = videoSettings->forceVideoDecoder()->rawValue().toInt();
    return settings;
}

VideoReceiver::DisplayConfig VideoStreamResolver::displayConfig(const Settings& settings)
{
    return {
        .conversionElement = settings.conversionElement,
        .disablePixelAspectRatio = settings.disablePixelAspectRatio,
        .gpuZeroCopy =
            !settings.forceCpuVideoPath && (settings.forceVideoDecoder != VideoDecoder::ForceVideoDecoderSoftware),
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
    config.timeout = uri.startsWith(QLatin1StringView("rtsp"), Qt::CaseInsensitive)
                         ? std::chrono::seconds(settings.rtspTimeoutSec)
                         : kNonRtspStreamTimeout;
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
