#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "VideoReceiver.h"

class QGCVideoStreamInfo;
class VideoSettings;

/// Maps the video settings and a camera's advertised stream to the stream a VideoReceiver plays.
namespace VideoStreamResolver {

enum class Role : uint8_t
{
    Primary,
    Thermal,
};

/// The video settings resolve() reads.
struct Settings
{
    QString videoSource;
    QString udpUrl;
    QString tcpUrl;
    QString rtspUrl;
    uint32_t rtspTimeoutSec = 8;
    bool lowLatency = false;
    int rtpJitterLatencyMs = 80;
    bool autoReconnect = true;
    QByteArray conversionElement;
    bool disablePixelAspectRatio = false;
    bool forceCpuVideoPath = false;
    int forceVideoDecoder = 0;  ///< A VideoDecoder::Options value.
};

Settings snapshot(VideoSettings* videoSettings);

/// The stream for @p role: the one @p streamInfo advertises, or else, for the primary role only, the manually
/// configured source. nullopt when there is none. Reads only its arguments: a camera's stream never changes the user's
/// settings.
std::optional<VideoReceiver::Config> resolve(const Settings& settings, const QGCVideoStreamInfo* streamInfo, Role role);

/// The display sink settings: GPU zero-copy unless the CPU path or a software decoder is forced.
VideoReceiver::DisplayConfig displayConfig(const Settings& settings);

/// True when @p streamInfo advertises a URI, which then takes precedence over the manual settings.
bool isAutoStream(const QGCVideoStreamInfo* streamInfo);

/// True for the manual sources a VideoReceiver plays (not "disabled", not a UVC camera).
bool isNetworkSource(const QString& videoSource);

/// How a stream arrives, from its URI scheme: "RTSP", "RTP/UDP (H.264)", "MPEG-TS/TCP", ...; the upper-cased scheme
/// for one resolve() never produces, empty for an empty @p uri.
QString transportName(const QString& uri);

}  // namespace VideoStreamResolver
