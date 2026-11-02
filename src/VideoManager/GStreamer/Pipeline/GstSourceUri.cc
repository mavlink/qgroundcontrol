#include "GstSourceUri.h"

#include <QtCore/QUrl>

namespace GStreamer::SourceFactory {

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

}  // namespace GStreamer::SourceFactory
