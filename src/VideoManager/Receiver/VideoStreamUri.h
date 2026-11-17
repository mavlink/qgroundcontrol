#pragma once

#include <cstdint>
#include <optional>

class QUrl;

/// The stream URIs VideoStreamResolver produces, as the receivers that play them tell them apart.
namespace VideoStreamUri {

enum class Transport : uint8_t
{
    Rtsp,        ///< Any rtsp* scheme: rtsp://, rtsps://, rtspt://, ...
    RtpUdpH264,  ///< udp://
    RtpUdpH265,  ///< udp265://
    MpegTsUdp,   ///< mpegts://
    MpegTsTcp,   ///< tcp://
};

/// The transport @p url's scheme selects, case-insensitively; nullopt for any other scheme.
std::optional<Transport> transportFor(const QUrl& url);

/// The receiver listens on a local UDP port for @p transport, rather than connect to a server.
constexpr bool listens(Transport transport)
{
    return (transport == Transport::RtpUdpH264) || (transport == Transport::RtpUdpH265) ||
           (transport == Transport::MpegTsUdp);
}

/// A TCP or UDP port a source can name: 1 to 65535.
constexpr bool isValidPort(int port)
{
    return (port > 0) && (port <= 65535);
}

}  // namespace VideoStreamUri
