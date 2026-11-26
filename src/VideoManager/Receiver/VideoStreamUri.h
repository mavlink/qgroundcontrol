#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QUrl>

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

/// What a stream URI lacks for a receiver to open it.
enum class AddressError : uint8_t
{
    InvalidPort,  ///< None, or one that is no port.
    MissingHost,
};

/// What @p url lacks as the address of a stream over @p transport; nullopt when a receiver can open it. A stream that
/// is listened for needs the port, where 0 stands for a free one, and may leave out the local address. MPEG-TS over
/// TCP needs the server's host and port. An RTSP URI is left to the RTSP client that opens it.
std::optional<AddressError> addressError(Transport transport, const QUrl& url);

/// What FFmpeg opens for a stream URI: the Qt Multimedia and FFmpeg receivers both play through it.
struct FFmpegSource
{
    /// What FFmpeg opens; for an SDP description, the name it is opened under.
    QUrl url;
    /// RTP over UDP: the SDP description FFmpeg reads in place of a URL.
    QByteArray sdp;
    /// nullopt for a URL that is none of VideoStreamResolver's, which FFmpeg opens as it is.
    std::optional<Transport> transport;
    /// RTSP interleaved over its TCP connection, rather than FFmpeg's choice, which tries UDP first.
    bool rtspOverTcp = false;

    /// An SDP names the streams, this one's or the RTSP server's: FFmpeg needn't read the stream to find them.
    bool streamsDescribed() const { return !sdp.isEmpty() || (transport == Transport::Rtsp); }
};

/// What FFmpeg opens for @p uri, one of VideoStreamResolver's: rtsp(s):// and tcp:// as they are, rtspt:// as rtsp://
/// over TCP, as GStreamer plays it, mpegts:// as udp://, and udp:// (H.264) or udp265:// (H.265) RTP, payload type 96,
/// as an SDP description. Other URLs as they are; nullopt for an invalid one, and for one of those whose address no
/// receiver opens (addressError()) or names port 0 to listen on.
std::optional<FFmpegSource> ffmpegSource(const QString& uri);

}  // namespace VideoStreamUri
