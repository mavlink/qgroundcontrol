#include "VideoStreamResolver.h"

#include <algorithm>
#include <climits>

#include "Fact.h"
#include "QGCVideoStreamInfo.h"
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

QString manualUri(const VideoStreamResolver::Settings& settings)
{
    const QString& source = settings.videoSource;
    if (source == VideoSettings::videoSourceUDPH264) {
        return addressWithScheme(QLatin1StringView("udp://"), settings.udpUrl);
    }
    if (source == VideoSettings::videoSourceUDPH265) {
        return addressWithScheme(QLatin1StringView("udp265://"), settings.udpUrl);
    }
    if (source == VideoSettings::videoSourceMPEGTS) {
        return addressWithScheme(QLatin1StringView("mpegts://"), settings.udpUrl);
    }
    if (source == VideoSettings::videoSourceRTSP) {
        return settings.rtspUrl;
    }
    if (source == VideoSettings::videoSourceTCP) {
        return addressWithScheme(QLatin1StringView("tcp://"), settings.tcpUrl);
    }
    if (source == VideoSettings::videoSource3DRSolo) {
        return QStringLiteral("udp://0.0.0.0:5600");
    }
    if (source == VideoSettings::videoSourceParrotDiscovery) {
        return QStringLiteral("udp://0.0.0.0:8888");
    }
    if (source == VideoSettings::videoSourceYuneecMantisG) {
        return QStringLiteral("rtsp://192.168.42.1:554/live");
    }
    if (source == VideoSettings::videoSourceHerelinkAirUnit) {
        return QStringLiteral("rtsp://192.168.0.10:8554/H264Video");
    }
    if (source == VideoSettings::videoSourceHerelinkHotspot) {
        return QStringLiteral("rtsp://192.168.43.1:8554/fpv_stream");
    }
    return QString();
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
    return settings;
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
    static const QStringList networkSources = {
        VideoSettings::videoSourceUDPH264,
        VideoSettings::videoSourceUDPH265,
        VideoSettings::videoSourceRTSP,
        VideoSettings::videoSourceTCP,
        VideoSettings::videoSourceMPEGTS,
        VideoSettings::videoSource3DRSolo,
        VideoSettings::videoSourceParrotDiscovery,
        VideoSettings::videoSourceYuneecMantisG,
        VideoSettings::videoSourceHerelinkAirUnit,
        VideoSettings::videoSourceHerelinkHotspot,
    };
    return networkSources.contains(videoSource);
}
