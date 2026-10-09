#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "UdpForwarder.h"
#include "UdpPeer.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QUdpSocket input;
    UdpForwarder forwarder;
    if (!input.bind(QHostAddress::LocalHost, 0) || forwarder.isEnabled() ||
        !forwarder.configure(QStringLiteral("127.0.0.1"), input.localPort()))
        return 1;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&input, &QUdpSocket::readyRead, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    const QByteArray payload("forwarded");
    if (forwarder.forward(payload) != payload.size())
        return 1;
    timeout.start(std::chrono::seconds(3));
    loop.exec();
    const QNetworkDatagram datagram = input.receiveDatagram();
    if (datagram.data() != payload)
        return 1;
    UdpDrainBudget budget;
    budget.consume(datagram.data().size());
    if (!budget.available() || udpPeerKey(datagram.senderAddress(), 1).isEmpty())
        return 1;
    forwarder.stop();
    return forwarder.isEnabled() || forwarder.forward(payload) != 0 ? 1 : 0;
}
