#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "VideoDecoderOptions.h"
#include "VideoReceiver.h"

class QGCVideoStreamInfo;
class VideoSettings;

/// Maps the video settings and a camera's advertised stream to what a stream source's VideoReceiver plays.
namespace VideoStreamResolver {
Q_NAMESPACE
QML_NAMED_ELEMENT(VideoSourceRole)

/// What a video source, each with a VideoStreamController, is. VideoSourceRole.Primary, ... in QML.
enum class Role : uint8_t
{
    /// The stream to display: a vehicle's current camera's current one, or its thermal one when it advertises no
    /// other, else the one the settings configure.
    Primary,
    Thermal,  ///< The thermal stream shown over the primary one: a vehicle's current camera's, else another camera's.
    Camera,   ///< The local camera: the one the video source setting names or, beside a stream, the one for a tile.
    Tile,     ///< Another stream a vehicle's cameras advertise, shown in a tile.
};
Q_ENUM_NS(Role)

/// The video settings that resolve(), forcedAspectRatio() and displayConfig() read.
struct Settings
{
    QString videoSource;
    QString udpUrl;
    QString tcpUrl;
    QString rtspUrl;
    uint32_t rtspTimeoutSec = 8;
    bool lowLatency = false;
    int rtpJitterLatencyMs = 80;
    int softwareDecoderThreads = 1;
    bool autoReconnect = true;
    QByteArray conversionElement;
    /// Width over height the configured source is stretched to; 0 for the video's own.
    double forcedAspectRatio = 0;
    bool forceCpuVideoPath = false;
    VideoDecoderOption forceVideoDecoder = VideoDecoderOption::Default;
};

Settings snapshot(VideoSettings* videoSettings);

/// What the source of @p role plays: the stream @p streamInfo advertises, or else, for the primary role only, the
/// manually configured one. nullopt when there is none, as for the camera role, which plays no stream. Reads only its
/// arguments: a camera's stream never changes the user's settings.
std::optional<VideoReceiver::Config> resolve(const Settings& settings, const QGCVideoStreamInfo* streamInfo, Role role);

/// Width over height the picture of the source of @p role is stretched to; 0 for the video's own. Only the manually
/// configured stream has one: a camera says what its streams are.
double forcedAspectRatio(const Settings& settings, const QGCVideoStreamInfo* streamInfo, Role role);

/// The display sink settings of the source of @p role: GPU zero-copy unless the CPU path or a software decoder is
/// forced, and no scaling to square pixels for a picture that is stretched to a forced ratio anyway.
VideoReceiver::DisplayConfig displayConfig(const Settings& settings, const QGCVideoStreamInfo* streamInfo, Role role);

/// True when @p streamInfo advertises a URI, which then takes precedence over the manual settings.
bool isAutoStream(const QGCVideoStreamInfo* streamInfo);

/// True for the manual sources that are network streams (not "disabled", not a local camera).
bool isNetworkSource(const QString& videoSource);

/// How a stream arrives, from its URI scheme: "RTSP", "RTP/UDP (H.264)", "MPEG-TS/TCP", ...; the upper-cased scheme
/// for one VideoStreamUri doesn't know, as a camera may advertise, and empty for an empty @p uri.
QString transportName(const QString& uri);

}  // namespace VideoStreamResolver
