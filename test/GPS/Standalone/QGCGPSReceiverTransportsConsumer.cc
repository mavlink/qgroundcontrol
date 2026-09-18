#include <QtCore/QCoreApplication>
#include <QtNetwork/QUdpSocket>

#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialGPSTransport.h"
#endif

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::atomic_bool stop = false;
    QUdpSocket peer;
    if (!peer.bind(QHostAddress::LocalHost, 0)) {
        return 1;
    }
    UDPGPSTransport udp(QStringLiteral("127.0.0.1"), peer.localPort(), stop);
    if (udp.open().status != GPSOpenStatus::Opened) {
        return 2;
    }
    const uint8_t bytes[]{1, 2, 3};
    const auto sent = udp.write(bytes, sizeof(bytes));
    if (sent.status != GPSWriteStatus::Completed || sent.writtenBytes != sizeof(bytes)) {
        return 3;
    }
    TCPGPSTransport tcp(QStringLiteral("localhost"), 1, stop);
#ifndef QGC_NO_SERIAL_LINK
    SerialGPSTransport serial(QStringLiteral("unused"), stop);
#endif
    stop = true;
    if (tcp.open().status != GPSOpenStatus::Cancelled) {
        return 4;
    }
#ifndef QGC_NO_SERIAL_LINK
    if (serial.open().status != GPSOpenStatus::Cancelled) {
        return 5;
    }
#endif
    return udp.write(bytes, sizeof(bytes)).status == GPSWriteStatus::Cancelled ? 0 : 6;
}
