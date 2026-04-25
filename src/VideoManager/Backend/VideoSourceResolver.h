#pragma once

#include <QtCore/QString>
#include <QtCore/QSize>

#include <optional>

#include "VideoReceiver.h"

class QGCVideoStreamInfo;
class VideoSettings;

/// Pure source-resolution logic extracted from VideoStream.
/// Maps settings + MAVLink auto-stream info → URI, and provides
/// scheme classification helpers. No lifecycle, no state.
namespace VideoSourceResolver {

/// Single source of truth for URI transport classification.
/// Replaces scattered isRtsp/isSrt/isWhep/isHlsOrDash/isPipelineString
/// predicates and gives GstSourceFactory a clean switch-based dispatch.
enum class Transport : quint8
{
    Unknown,
    Pipeline,  ///< gstreamer-pipeline:desc
    RTSP,      ///< rtsp://, rtsps://
    UDP_H264,  ///< udp://
    UDP_H265,  ///< udp265://
    MPEGTS,    ///< mpegts://
    TCP,       ///< tcp://   (MPEG-TS over TCP)
    SRT,       ///< srt://
    WHEP,      ///< whep://, wheps://
    HLS,       ///< hls://, hlss://
    DASH,      ///< dash://, dashs://
};

/// Value object describing a resolved video source. This is the handoff object
/// for lifecycle, recording, and playback policy so those layers do not repeat
/// transport predicates independently.
struct VideoSource
{
    QString uri;
    QString sourceName;  ///< Settings-persistence key (empty = no writeback needed)
    Transport transport = Transport::Unknown;

    /// True when this source needs GStreamer-side transport normalization
    /// before Qt Multimedia receives the playback input.
    bool requiresIngestSession = false;
    bool isLocalCamera = false;
    QString localCameraId;
    bool isNetwork = false;
    bool isPipeline = false;
    bool lowLatencyRecommended = false;
    VideoReceiver::PlaybackPolicy playbackPolicy;
    int startupTimeoutS = 3;
    bool changed = false;

    [[nodiscard]] bool isValid() const { return !uri.isEmpty() || isLocalCamera; }
    [[nodiscard]] bool needsIngestSession() const { return requiresIngestSession; }
    [[nodiscard]] bool isRtsp() const { return transport == Transport::RTSP; }
    [[nodiscard]] uint32_t startupTimeout() const { return static_cast<uint32_t>(startupTimeoutS); }
};

using SourceDescriptor = VideoSource;

struct EffectiveSource
{
    bool hasSource = false;
    SourceDescriptor source;
    QString writeBackSourceName;
    QString writeBackRtspUrl;
};

struct StreamInfo
{
    QString uri;
    QString name;
    std::optional<quint8> streamID;
    quint8 type = 0;
    quint8 encoding = 0;
    bool thermal = false;
    bool active = false;
    QSize resolution;
    quint16 hfov = 0;
    quint16 rotation = 0;
    quint32 bitrate = 0;
    qreal framerate = 0.0;

    [[nodiscard]] bool isValid() const { return streamID.has_value() || !uri.isEmpty(); }
};

/// Classify a URI into a Transport. Side-effect free.
Transport classify(const QString& uri);

/// Resolve from user settings (table-driven + special cases).
SourceDescriptor fromSettings(VideoSettings* settings);

/// Resolve from a MAVLink VIDEO_STREAM_INFORMATION snapshot.
SourceDescriptor fromStreamInfo(const StreamInfo& info);

/// Snapshot MAVLink VIDEO_STREAM_INFORMATION into a value object so source
/// resolution does not depend on raw QGCVideoStreamInfo ownership.
std::optional<StreamInfo> streamInfoFrom(const QGCVideoStreamInfo* info);

/// Resolve the effective source for a stream, preferring MAVLink
/// VIDEO_STREAM_INFORMATION over manual settings. Primary streams may request
/// settings writeback from auto-discovered sources; thermal streams never do.
EffectiveSource resolveEffectiveSource(bool thermal,
                                       VideoSettings* settings,
                                       const std::optional<StreamInfo>& streamInfo);

/// Build a source descriptor from a raw URI.
SourceDescriptor describeUri(const QString& uri, const QString& sourceName = QString(), bool changed = false);

/// Returns true if \a sourceName is a known network-stream source.
bool isKnownNetworkSource(const QString& sourceName);

}  // namespace VideoSourceResolver
