#pragma once

#include <cstdint>
#include <optional>

class QUrl;

/// The video URIs GStreamer::SourceFactory::create() accepts, without GStreamer.
namespace GStreamer::SourceFactory {

enum class Transport : uint8_t
{
    Rtsp,        ///< Any rtsp* scheme: rtsp://, rtspt://, ...
    RtpUdpH264,  ///< udp://
    RtpUdpH265,  ///< udp265://
    MpegTsUdp,   ///< mpegts://
    MpegTsTcp,   ///< tcp://
};

/// The transport for @p url's scheme, case-insensitively; nullopt when create() doesn't support it.
std::optional<Transport> transportFor(const QUrl& url);

constexpr bool isMpegTs(Transport transport)
{
    return (transport == Transport::MpegTsUdp) || (transport == Transport::MpegTsTcp);
}

/// A TCP or UDP port a source can name: 1 to 65535.
constexpr bool isValidPort(int port)
{
    return (port > 0) && (port <= 65535);
}

}  // namespace GStreamer::SourceFactory
