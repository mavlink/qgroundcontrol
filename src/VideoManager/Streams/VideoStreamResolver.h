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

/// Maps the video settings and a camera's advertised stream to what a video source's VideoReceiver plays.
namespace VideoStreamResolver {
Q_NAMESPACE
QML_NAMED_ELEMENT(VideoSourceRole)

/// The video sources, each with a VideoStreamController. VideoSourceRole.Primary, ... in QML.
enum class Role : uint8_t
{
    Primary,  ///< The stream on display: the one a vehicle's camera advertises, else the one the settings configure.
    Thermal,  ///< The thermal stream a vehicle's camera advertises.
    Camera,   ///< The local camera the video source setting names.
};
Q_ENUM_NS(Role)

/// The video settings resolve() reads, and what they depend on.
struct Settings
{
    QString videoSource;
    /// videoSource names a local camera that is attached.
    bool cameraAttached = false;
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
    VideoDecoderOption forceVideoDecoder = VideoDecoderOption::Default;
};

Settings snapshot(VideoSettings* videoSettings);

/// What the source of @p role plays. A stream role: the stream @p streamInfo advertises, or else, for the primary role
/// only, the manually configured one. The camera role: the attached camera the video source names, as the URI.
/// nullopt when there is none. Reads only its arguments: a camera's stream never changes the user's settings.
std::optional<VideoReceiver::Config> resolve(const Settings& settings, const QGCVideoStreamInfo* streamInfo, Role role);

/// The display sink settings: GPU zero-copy unless the CPU path or a software decoder is forced.
VideoReceiver::DisplayConfig displayConfig(const Settings& settings);

/// True when @p streamInfo advertises a URI, which then takes precedence over the manual settings.
bool isAutoStream(const QGCVideoStreamInfo* streamInfo);

/// True for the manual sources that are network streams (not "disabled", not a local camera).
bool isNetworkSource(const QString& videoSource);

/// How a stream arrives, from its URI scheme: "RTSP", "RTP/UDP (H.264)", "MPEG-TS/TCP", ...; the upper-cased scheme
/// for one resolve() never produces, empty for an empty @p uri.
QString transportName(const QString& uri);

}  // namespace VideoStreamResolver
