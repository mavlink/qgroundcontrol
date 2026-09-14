#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include "UdpForwarder.h"
#include "UdpIODevice.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    UdpIODevice input;
    UdpForwarder forwarder;
    if (!input.bind(QHostAddress::LocalHost, 0) || forwarder.isEnabled() ||
        !input.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
        return 1;
    QUdpSocket sender;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&input, &QIODevice::readyRead, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    const auto send = [&](const QByteArray& data) {
        if (sender.writeDatagram(data, QHostAddress::LocalHost, input.localPort()) != data.size())
            return false;
        timeout.start(std::chrono::seconds(3));
        loop.exec();
        const bool received = timeout.isActive();
        timeout.stop();
        return received;
    };
    const auto beforeFirst = ReadTimestamp::nowUs();
    if (!send("first\n") || input.read(2) != "fi")
        return 1;
    const auto firstReceipt = ReadTimestamp::from(&input);
    const auto beforeSecond = ReadTimestamp::nowUs();
    if (firstReceipt < beforeFirst || firstReceipt > beforeSecond || !send("next\n"))
        return 1;
    if (input.read(1024) != "rst\n" || ReadTimestamp::from(&input) != firstReceipt)
        return 1;
    if (input.read(1024) != "next\n" || ReadTimestamp::from(&input) < beforeSecond ||
        ReadTimestamp::from(&input) > ReadTimestamp::nowUs())
        return 1;
    input.close();
    return input.bytesAvailable() == 0 && ReadTimestamp::from(&input) == 0 ? 0 : 1;
}
