#pragma once

#include <QtCore/QString>

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
    Pipeline,  ///< gstreamer-pipeline:desc  (Qt canonical + legacy `//` form)
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

enum class FrameMemoryPreference : quint8
{
    Cpu,
    Platform,
};

/// Value object describing a resolved video source. This is the handoff object
/// for lifecycle, backend, recording, and platform-memory policy so those
/// layers do not repeat transport/backend predicates independently.
struct VideoSource
{
    QString uri;
    QString sourceName;  ///< Settings-persistence key (empty = no writeback needed)
    Transport transport = Transport::Unknown;
    VideoReceiver::BackendKind preferredBackend = VideoReceiver::BackendKind::QtMultimedia;
    bool requiresGStreamer = false;
    bool isLocalCamera = false;
    bool isNetwork = false;
    bool isPipeline = false;
    bool lowLatencyRecommended = false;
    bool supportsLosslessRecording = false;
    FrameMemoryPreference frameMemoryPreference = FrameMemoryPreference::Cpu;
    int startupTimeoutS = 3;
    bool changed = false;

    [[nodiscard]] bool isValid() const { return !uri.isEmpty() || isLocalCamera; }
    [[nodiscard]] bool usesGStreamer() const { return requiresGStreamer; }
    [[nodiscard]] bool usesPlatformFrames() const { return frameMemoryPreference == FrameMemoryPreference::Platform; }
    [[nodiscard]] bool isRtsp() const { return transport == Transport::RTSP; }
    [[nodiscard]] bool canRecordLosslessly() const { return supportsLosslessRecording; }
    [[nodiscard]] uint32_t startupTimeout() const { return static_cast<uint32_t>(startupTimeoutS); }
    [[nodiscard]] VideoReceiver::BackendKind backend() const { return preferredBackend; }
};

using SourceDescriptor = VideoSource;

/// Classify a URI into a Transport. Side-effect free.
Transport classify(const QString& uri);

/// Resolve from user settings (table-driven + special cases).
SourceDescriptor fromSettings(VideoSettings* settings);

/// Resolve from MAVLink VIDEO_STREAM_INFORMATION.
SourceDescriptor fromStreamInfo(const QGCVideoStreamInfo* info);

/// Build a source descriptor from a raw URI.
SourceDescriptor describeUri(const QString& uri, const QString& sourceName = QString(), bool changed = false);

/// Returns true if \a sourceName is a known network-stream source.
bool isKnownNetworkSource(const QString& sourceName);

}  // namespace VideoSourceResolver
