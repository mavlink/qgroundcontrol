#pragma once

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

namespace GPSTest {

/// A UDP port that was free on the dual-stack wildcard address, and so on every interface, or 0 if none could be
/// bound. The probe releases the port before returning, so bind it promptly.
inline quint16 unusedUdpPort()
{
    QUdpSocket probe;
    return probe.bind(QHostAddress::Any, 0) ? probe.localPort() : 0;
}

/// Calls @a bind with ports unusedUdpPort() finds until it binds one, as another process can take a probed port before
/// it is bound. @a failures counts the binds that failed. @return the bound port, or 0 when every attempt failed.
template <typename Bind>
quint16 bindUnusedUdpPort(Bind bind, int& failures, int attempts = 5)
{
    for (int attempt = 0; attempt < attempts; ++attempt) {
        const quint16 port = unusedUdpPort();
        if (port != 0 && bind(port)) {
            return port;
        }
        ++failures;
    }
    return 0;
}

}  // namespace GPSTest
