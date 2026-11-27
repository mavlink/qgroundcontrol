#include "VideoStreamUri.h"

#include <QtCore/QUrl>

namespace {

/// An SDP description of a stream of @p encoding RTP packets, payload type 96, arriving at @p port.
QByteArray rtpSdp(const QString& host, int port, QByteArrayView encoding)
{
    const QByteArray address = (host.isEmpty() ? QStringLiteral("0.0.0.0") : host).toLatin1();
    return QByteArrayLiteral("v=0\r\n") + "o=- 0 0 IN IP4 " + address + "\r\ns=QGroundControl\r\nc=IN IP4 " + address +
           "\r\nt=0 0\r\nm=video " + QByteArray::number(port) + " RTP/AVP 96\r\na=rtpmap:96 " + encoding + "/90000\r\n";
}

}  // namespace

namespace VideoStreamUri {

std::optional<Transport> transportFor(const QUrl& url)
{
    const QString scheme = url.scheme().toLower();
    if (scheme.startsWith(QLatin1String("rtsp"))) {
        return Transport::Rtsp;
    }
    if (scheme == QLatin1String("udp")) {
        return Transport::RtpUdpH264;
    }
    if (scheme == QLatin1String("udp265")) {
        return Transport::RtpUdpH265;
    }
    if (scheme == QLatin1String("mpegts")) {
        return Transport::MpegTsUdp;
    }
    if (scheme == QLatin1String("tcp")) {
        return Transport::MpegTsTcp;
    }
    return std::nullopt;
}

std::optional<AddressError> addressError(Transport transport, const QUrl& url)
{
    const int port = url.port();
    switch (transport) {
        case Transport::Rtsp:
            break;
        case Transport::RtpUdpH264:
        case Transport::RtpUdpH265:
        case Transport::MpegTsUdp:
            if ((port != 0) && !isValidPort(port)) {
                return AddressError::InvalidPort;
            }
            break;
        case Transport::MpegTsTcp:
            if (!isValidPort(port)) {
                return AddressError::InvalidPort;
            }
            if (url.host().isEmpty()) {
                return AddressError::MissingHost;
            }
            break;
    }
    return std::nullopt;
}

std::optional<FFmpegSource> ffmpegSource(const QString& uri)
{
    const QUrl url(uri);
    if (uri.isEmpty() || !url.isValid() || url.scheme().isEmpty()) {
        return std::nullopt;
    }

    const std::optional<Transport> transport = transportFor(url);
    // A free port (0) is no use to listen on: FFmpeg doesn't tell which one it got, for the sender to be given.
    if (transport && (addressError(*transport, url) || (listens(*transport) && (url.port() == 0)))) {
        return std::nullopt;
    }

    FFmpegSource source{.url = url, .sdp = {}, .transport = transport};
    if ((transport == Transport::RtpUdpH264) || (transport == Transport::RtpUdpH265)) {
        source.url = QUrl(QStringLiteral("stream.sdp"));
        source.sdp = rtpSdp(url.host(), url.port(), (transport == Transport::RtpUdpH265) ? "H265" : "H264");
    } else if (transport == Transport::MpegTsUdp) {
        source.url.setScheme(QStringLiteral("udp"));
    } else if ((transport == Transport::Rtsp) &&
               (url.scheme().compare(QStringLiteral("rtspt"), Qt::CaseInsensitive) == 0)) {
        source.url.setScheme(QStringLiteral("rtsp"));
        source.rtspOverTcp = true;
    }
    return source;
}

}  // namespace VideoStreamUri
