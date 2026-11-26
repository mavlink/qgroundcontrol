#include "VideoStreamUri.h"

#include <QtCore/QUrl>

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

}  // namespace VideoStreamUri
