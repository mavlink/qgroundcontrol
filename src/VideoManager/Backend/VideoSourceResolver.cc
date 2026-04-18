#include "VideoSourceResolver.h"

#include "QGCVideoStreamInfo.h"
#include "VideoSettings.h"

using ST = VideoSettings::SourceType;

// ═══════════════════════════════════════════════════════════════════════════
// Settings source table — maps SourceType → URI template + Fact accessor
// ═══════════════════════════════════════════════════════════════════════════

struct SettingsSourceEntry
{
    ST type;
    const char* sourceName;
    const char* uriTemplate;                // %1 → setting value, or literal
    Fact* (VideoSettings::*settingFact)();  // Fact providing %1 (nullptr → literal)
};

static constexpr SettingsSourceEntry kSettingsSources[] = {
    {ST::UDPH264, VideoSettings::videoSourceUDPH264, "udp://%1", &VideoSettings::udpUrl},
    {ST::UDPH265, VideoSettings::videoSourceUDPH265, "udp265://%1", &VideoSettings::udpUrl},
    {ST::MPEGTS, VideoSettings::videoSourceMPEGTS, "mpegts://%1", &VideoSettings::udpUrl},
    {ST::RTSP, VideoSettings::videoSourceRTSP, "%1", &VideoSettings::rtspUrl},
    {ST::TCP, VideoSettings::videoSourceTCP, "tcp://%1", &VideoSettings::tcpUrl},
    {ST::Solo3DR, VideoSettings::videoSource3DRSolo, "udp://0.0.0.0:5600", nullptr},
    {ST::ParrotDiscovery, VideoSettings::videoSourceParrotDiscovery, "udp://0.0.0.0:8888", nullptr},
    {ST::YuneecMantisG, VideoSettings::videoSourceYuneecMantisG, "rtsp://192.168.42.1:554/live", nullptr},
    {ST::HerelinkAirUnit, VideoSettings::videoSourceHerelinkAirUnit, "rtsp://192.168.0.10:8554/H264Video", nullptr},
    {ST::HerelinkHotspot, VideoSettings::videoSourceHerelinkHotspot, "rtsp://192.168.43.1:8554/fpv_stream", nullptr},
};

// ═══════════════════════════════════════════════════════════════════════════
// MAVLink auto-stream table — maps (streamType, encoding) → SourceType + URI prefix
// Unifies the old _updateAutoStream switch statement with the settings table.
// ═══════════════════════════════════════════════════════════════════════════

struct AutoStreamEntry
{
    int streamType;      // VIDEO_STREAM_TYPE_*
    int encoding;        // VIDEO_STREAM_ENCODING_* (-1 = match any)
    ST sourceType;
    const char* scheme;  // prepend "scheme://0.0.0.0:" if URI is bare (port only)
};

static constexpr AutoStreamEntry kAutoStreamSources[] = {
    {VIDEO_STREAM_TYPE_RTSP, -1, ST::RTSP, nullptr},
    {VIDEO_STREAM_TYPE_TCP_MPEG, -1, ST::TCP, nullptr},
    {VIDEO_STREAM_TYPE_RTPUDP, VIDEO_STREAM_ENCODING_H265, ST::UDPH265, "udp265://"},
    {VIDEO_STREAM_TYPE_RTPUDP, VIDEO_STREAM_ENCODING_H264, ST::UDPH264, "udp://"},
    {VIDEO_STREAM_TYPE_MPEG_TS, -1, ST::MPEGTS, "mpegts://"},
};

// ═══════════════════════════════════════════════════════════════════════════
// Transport classification table
// ═══════════════════════════════════════════════════════════════════════════

// Prefix-ordered: longer/more-specific prefixes first to avoid mis-matches
// (e.g. "udp265://" must check before "udp://", "gstreamer-pipeline:" must
// check before any scheme that might incidentally contain ':').
struct TransportPrefix
{
    const char* prefix;
    VideoSourceResolver::Transport transport;
};

static constexpr TransportPrefix kTransportPrefixes[] = {
    {"gstreamer-pipeline:", VideoSourceResolver::Transport::Pipeline},
    {"rtsps://", VideoSourceResolver::Transport::RTSP},
    {"rtsp://", VideoSourceResolver::Transport::RTSP},
    {"udp265://", VideoSourceResolver::Transport::UDP_H265},
    {"udp://", VideoSourceResolver::Transport::UDP_H264},
    {"mpegts://", VideoSourceResolver::Transport::MPEGTS},
    {"tcp://", VideoSourceResolver::Transport::TCP},
    {"srt://", VideoSourceResolver::Transport::SRT},
    {"wheps://", VideoSourceResolver::Transport::WHEP},
    {"whep://", VideoSourceResolver::Transport::WHEP},
    {"hlss://", VideoSourceResolver::Transport::HLS},
    {"hls://", VideoSourceResolver::Transport::HLS},
    {"dashs://", VideoSourceResolver::Transport::DASH},
    {"dash://", VideoSourceResolver::Transport::DASH},
};

// ═══════════════════════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════════════════════

VideoSourceResolver::Transport VideoSourceResolver::classify(const QString& uri)
{
    for (const auto& entry : kTransportPrefixes) {
        if (uri.startsWith(QLatin1String(entry.prefix), Qt::CaseInsensitive))
            return entry.transport;
    }
    return Transport::Unknown;
}

VideoSourceResolver::SourceDescriptor VideoSourceResolver::describeUri(const QString& uri,
                                                                       const QString& sourceName,
                                                                       bool changed)
{
    const Transport transport = classify(uri);
    const bool isLocalCamera = uri.startsWith(QLatin1String("uvc://"), Qt::CaseInsensitive);
    const VideoReceiver::BackendKind backend = isLocalCamera
                                                   ? VideoReceiver::BackendKind::UVC
                                                   : ((transport != Transport::Unknown)
                                                          ? VideoReceiver::BackendKind::GStreamer
                                                          : VideoReceiver::BackendKind::QtMultimedia);
    const bool requiresGStreamer = backend == VideoReceiver::BackendKind::GStreamer;
    const bool isNetwork = transport != Transport::Unknown && transport != Transport::Pipeline;
    const bool isPipeline = transport == Transport::Pipeline;
    SourceDescriptor source;
    source.uri = uri;
    source.sourceName = sourceName;
    source.transport = transport;
    source.preferredBackend = backend;
    source.requiresGStreamer = requiresGStreamer;
    source.isLocalCamera = isLocalCamera;
    source.isNetwork = isNetwork;
    source.isPipeline = isPipeline;
    source.lowLatencyRecommended = requiresGStreamer;
    source.supportsLosslessRecording = requiresGStreamer;
    source.frameMemoryPreference = requiresGStreamer ? FrameMemoryPreference::Platform : FrameMemoryPreference::Cpu;
    source.startupTimeoutS = transport == Transport::RTSP ? 8 : 3;
    source.changed = changed;
    return source;
}

VideoSourceResolver::SourceDescriptor VideoSourceResolver::fromSettings(VideoSettings* settings)
{
    const auto sourceType = settings->currentSourceType();

    // Table-driven lookup for standard network sources
    for (const auto& entry : kSettingsSources) {
        if (sourceType == entry.type) {
            const QString uri =
                entry.settingFact
                    ? QString(entry.uriTemplate).arg((settings->*entry.settingFact)()->rawValue().toString())
                    : QString::fromLatin1(entry.uriTemplate);
            return describeUri(uri, QString::fromLatin1(entry.sourceName), true);
        }
    }

    // Non-table sources
    switch (sourceType) {
        case ST::GstPipeline: {
            const QString pipeline = settings->gstPipelineUrl()->rawValue().toString();
            const QString uri = pipeline.isEmpty() ? QString() : QStringLiteral("gstreamer-pipeline://") + pipeline;
            return describeUri(uri, QString::fromLatin1(VideoSettings::videoSourceGstPipeline), true);
        }
        case ST::UVC: {
            SourceDescriptor source = describeUri(QString(), QString(), true);
            source.preferredBackend = VideoReceiver::BackendKind::UVC;
            source.isLocalCamera = true;
            source.supportsLosslessRecording = false;
            source.frameMemoryPreference = FrameMemoryPreference::Cpu;
            return source;
        }
        case ST::Disabled:
        case ST::NoVideo:
        case ST::Unknown:
        default:
            return describeUri(QString(), QString(), true);
    }
}

VideoSourceResolver::SourceDescriptor VideoSourceResolver::fromStreamInfo(const QGCVideoStreamInfo* info)
{
    if (!info || info->uri().isEmpty())
        return {};

    for (const auto& entry : kAutoStreamSources) {
        if (info->type() != entry.streamType)
            continue;
        if (entry.encoding >= 0 && info->encoding() != entry.encoding)
            continue;

        // Find the settings source name for this source type
        const char* sourceName = nullptr;
        for (const auto& se : kSettingsSources) {
            if (se.type == entry.sourceType) {
                sourceName = se.sourceName;
                break;
            }
        }

        // Build URI: if the auto-stream info already has a full URI, use it.
        // Otherwise prepend scheme + default host.
        QString uri = info->uri();
        if (entry.scheme && !uri.contains(QLatin1String(entry.scheme), Qt::CaseInsensitive)) {
            uri = QStringLiteral("%1%2:%3").arg(QLatin1String(entry.scheme), QStringLiteral("0.0.0.0"), info->uri());
        }

        return describeUri(uri, sourceName ? QString::fromLatin1(sourceName) : QString(), true);
    }

    // Unknown stream type — return raw URI with no source name
    const QString uri = info->uri();
    return describeUri(uri, QString::fromLatin1(VideoSettings::videoSourceNoVideo), true);
}

bool VideoSourceResolver::isKnownNetworkSource(const QString& sourceName)
{
    for (const auto& entry : kSettingsSources) {
        if (sourceName == QLatin1String(entry.sourceName))
            return true;
    }
    return false;
}
