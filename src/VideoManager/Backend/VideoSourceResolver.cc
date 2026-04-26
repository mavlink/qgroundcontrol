#include "VideoSourceResolver.h"

#include "QGCVideoStreamInfo.h"
#include "VideoSourceCatalog.h"
#include "VideoSettings.h"

#include <QtCore/QUrl>

using ST = VideoSettings::SourceType;

// ═══════════════════════════════════════════════════════════════════════════
// Settings source table — maps SourceType → URI template + Fact accessor
// ═══════════════════════════════════════════════════════════════════════════

struct SettingsSourceEntry
{
    ST type;
    const char* uriTemplate;                // %1 → setting value, or literal
    Fact* (VideoSettings::*settingFact)();  // Fact providing %1 (nullptr → literal)
};

static constexpr SettingsSourceEntry kSettingsSources[] = {
    {ST::UDPH264, "udp://%1", &VideoSettings::udpUrl},
    {ST::UDPH265, "udp265://%1", &VideoSettings::udpUrl},
    {ST::MPEGTS, "mpegts://%1", &VideoSettings::udpUrl},
    {ST::RTSP, "%1", &VideoSettings::rtspUrl},
    {ST::TCP, "tcp://%1", &VideoSettings::tcpUrl},
    {ST::Solo3DR, "udp://0.0.0.0:5600", nullptr},
    {ST::ParrotDiscovery, "udp://0.0.0.0:8888", nullptr},
    {ST::YuneecMantisG, "rtsp://192.168.42.1:554/live", nullptr},
    {ST::HerelinkAirUnit, "rtsp://192.168.0.10:8554/H264Video", nullptr},
    {ST::HerelinkHotspot, "rtsp://192.168.43.1:8554/fpv_stream", nullptr},
};

// ═══════════════════════════════════════════════════════════════════════════
// MAVLink auto-stream table — maps (streamType, encoding) → SourceType + URI prefix
// Unifies MAVLink auto-stream classification with the settings source table.
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

QString encodeLocalCameraId(const QString& cameraId)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(cameraId));
}

QString localCameraIdFromUri(const QString& uri)
{
    static constexpr auto kPrefix = "uvc://";
    if (!uri.startsWith(QLatin1String(kPrefix), Qt::CaseInsensitive))
        return {};

    QString encoded = uri.mid(6);
    while (encoded.startsWith(QLatin1Char('/')))
        encoded.remove(0, 1);

    const QString cameraId = QUrl::fromPercentEncoding(encoded.toUtf8());
    return cameraId == QLatin1String("local") ? QString() : cameraId;
}

// ═══════════════════════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════════════════════

VideoSourceResolver::Transport VideoSourceResolver::classify(const QString& uri)
{
    if (uri.startsWith(QLatin1String("gstreamer-pipeline://"), Qt::CaseInsensitive))
        return Transport::Unknown;

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
    const bool isPipeline = transport == Transport::Pipeline;
    const bool qtCanOpenDirectly = transport == Transport::HLS || transport == Transport::DASH;
    const bool requiresIngestSession = transport != Transport::Unknown && !qtCanOpenDirectly;
    const bool isNetwork = transport != Transport::Unknown && transport != Transport::Pipeline;
    SourceDescriptor source;
    source.uri = uri;
    source.sourceName = sourceName;
    source.transport = transport;
    source.requiresIngestSession = requiresIngestSession;
    source.isLocalCamera = isLocalCamera;
    source.localCameraId = localCameraIdFromUri(uri);
    source.isNetwork = isNetwork;
    source.isPipeline = isPipeline;
    source.lowLatencyRecommended = requiresIngestSession;
    source.playbackPolicy.lowLatencyStreaming = requiresIngestSession;
    source.playbackPolicy.probeSizeBytes = isNetwork ? 32768 : 0;
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
            return describeUri(uri, VideoSourceCatalog::sourceNameForType(entry.type), true);
        }
    }

    // Non-table sources
    switch (sourceType) {
        case ST::GstPipeline: {
            const QString pipeline = settings->gstPipelineUrl()->rawValue().toString();
            const QString uri = pipeline.isEmpty() ? QString() : QStringLiteral("gstreamer-pipeline:") + pipeline;
            return describeUri(uri, VideoSourceCatalog::sourceNameForType(ST::GstPipeline), true);
        }
        case ST::UVC: {
            const QString cameraId = settings->videoSource()->rawValue().toString();
            const QString uri = cameraId.isEmpty()
                                    ? QStringLiteral("uvc://local")
                                    : QStringLiteral("uvc://") + encodeLocalCameraId(cameraId);
            SourceDescriptor source = describeUri(uri, QString(), true);
            source.isLocalCamera = true;
            source.localCameraId = cameraId;
            return source;
        }
        case ST::Disabled:
        case ST::NoVideo:
        case ST::Unknown:
        default:
            return describeUri(QString(), QString(), true);
    }
}

std::optional<VideoSourceResolver::StreamInfo> VideoSourceResolver::streamInfoFrom(const QGCVideoStreamInfo* info)
{
    if (!info)
        return std::nullopt;

    StreamInfo snapshot;
    snapshot.uri = info->uri();
    snapshot.name = info->name();
    snapshot.streamID = info->streamID();
    snapshot.type = info->type();
    snapshot.encoding = info->encoding();
    snapshot.thermal = info->isThermal();
    snapshot.active = info->isActive();
    snapshot.resolution = info->resolution();
    snapshot.hfov = info->hfov();
    snapshot.rotation = info->rotation();
    snapshot.bitrate = info->bitrate();
    snapshot.framerate = info->framerate();
    return snapshot;
}

VideoSourceResolver::SourceDescriptor VideoSourceResolver::fromStreamInfo(const StreamInfo& info)
{
    if (info.uri.isEmpty())
        return {};

    for (const auto& entry : kAutoStreamSources) {
        if (info.type != entry.streamType)
            continue;
        if (entry.encoding >= 0 && info.encoding != entry.encoding)
            continue;

        // Find the settings source name for this source type
        const QLatin1String sourceName = VideoSourceCatalog::sourceNameForType(entry.sourceType);

        // Build URI: if the auto-stream info already has a full URI, use it.
        // Otherwise prepend scheme + default host.
        QString uri = info.uri;
        if (entry.scheme && !uri.contains(QLatin1String(entry.scheme), Qt::CaseInsensitive)) {
            uri = QStringLiteral("%1%2:%3").arg(QLatin1String(entry.scheme), QStringLiteral("0.0.0.0"), info.uri);
        }

        return describeUri(uri, sourceName, true);
    }

    // Unknown stream type — return raw URI with no source name
    const QString uri = info.uri;
    return describeUri(uri, VideoSourceCatalog::sourceNameForType(ST::NoVideo), true);
}

VideoSourceResolver::EffectiveSource
VideoSourceResolver::resolveEffectiveSource(bool thermal,
                                            VideoSettings* settings,
                                            const std::optional<StreamInfo>& streamInfo)
{
    EffectiveSource resolution;

    if (streamInfo) {
        resolution.source = fromStreamInfo(*streamInfo);
        resolution.hasSource = true;
        if (!thermal) {
            resolution.writeBackSourceName = resolution.source.sourceName;
            if (resolution.source.sourceName == QLatin1String(VideoSettings::videoSourceRTSP))
                resolution.writeBackRtspUrl = resolution.source.uri;
        }
        return resolution;
    }

    if (!thermal && settings) {
        resolution.source = fromSettings(settings);
        resolution.hasSource = true;
    }

    return resolution;
}

bool VideoSourceResolver::isKnownNetworkSource(const QString& sourceName)
{
    return VideoSourceCatalog::isNetworkSourceName(sourceName);
}
