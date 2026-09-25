#include "UDPGPSTransportTest.h"

#include <array>
#include <atomic>

#include <QtCore/QRegularExpression>
#include <QtNetwork/QUdpSocket>

#include "UDPGPSTransport.h"

namespace {
quint16 reserveUdpPort()
{
    QUdpSocket reservation;
    if (!reservation.bind(QHostAddress::LocalHost, 0)) {
        return 0;
    }
    const quint16 port = reservation.localPort();
    reservation.close();
    return port;
}

QByteArray readAll(UDPGPSTransport& transport, int timeoutMs)
{
    std::array<uint8_t, 256> buffer{};
    QByteArray received;
    for (;;) {
        const auto result = transport.read(buffer.data(), static_cast<int>(buffer.size()), timeoutMs);
        if (result.status != GPSReadStatus::Data || result.bytesRead == 0) {
            return received;
        }
        received.append(reinterpret_cast<const char*>(buffer.data()), result.bytesRead);
        timeoutMs = 0;
    }
}
}  // namespace

void UDPGPSTransportTest::_receivesSelectedSender()
{
    std::atomic_bool stop = false;
    const quint16 port = reserveUdpPort();
    QVERIFY(port != 0);
    UDPGPSTransport transport(port, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QVERIFY(!transport.fatalError());
    QUdpSocket first;
    QUdpSocket second;
    QVERIFY(first.bind(QHostAddress::LocalHost, 0));
    QVERIFY(second.bind(QHostAddress::LocalHost, 0));
    QCOMPARE(first.writeDatagram("$GPGGA,1*", QHostAddress::LocalHost, port), 9);
    QCOMPARE(second.writeDatagram("ignored", QHostAddress::LocalHost, port), 7);
    QCOMPARE(first.writeDatagram("00\r\n", QHostAddress::LocalHost, port), 4);
    QByteArray received;
    QTRY_COMPARE_WITH_TIMEOUT((received += readAll(transport, 20), received), QByteArray("$GPGGA,1*00\r\n"),
                              TestTimeout::mediumMs());
    QCOMPARE(readAll(transport, 0), QByteArray());
}

void UDPGPSTransportTest::_idleSenderIsReplaced()
{
    std::atomic_bool stop = false;
    const quint16 port = reserveUdpPort();
    QVERIFY(port != 0);
    UDPGPSTransport transport(port, stop, 50);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QUdpSocket first;
    QUdpSocket second;
    QVERIFY(first.bind(QHostAddress::LocalHost, 0));
    QVERIFY(second.bind(QHostAddress::LocalHost, 0));
    QCOMPARE(first.writeDatagram("$partial", QHostAddress::LocalHost, port), 8);
    QByteArray received;
    QTRY_COMPARE_WITH_TIMEOUT((received += readAll(transport, 20), received), QByteArray("$partial"),
                              TestTimeout::mediumMs());
    // Once the selected sender is idle, another sender takes over without inheriting its bytes.
    QTRY_VERIFY_WITH_TIMEOUT(
        second.writeDatagram("$next", QHostAddress::LocalHost, port) == 5 && readAll(transport, 20).endsWith("$next"),
        TestTimeout::mediumMs());
}

void UDPGPSTransportTest::_receiveOnlyLink()
{
    std::atomic_bool stop = false;
    UDPGPSTransport transport(0, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QCOMPARE(transport.fixedBaudrate(), UDPGPSTransport::FIXED_BAUDRATE);
    QVERIFY(transport.setBaudrate(UDPGPSTransport::FIXED_BAUDRATE));
    QVERIFY(!transport.setBaudrate(9600));
    const uint8_t command[] = {'$', 'P'};
    const auto written = transport.write(command, sizeof(command), QDeadlineTimer(100));
    QCOMPARE(written.status, GPSWriteStatus::Unsupported);
    QCOMPARE(written.acceptedBytes, 0);
    std::array<uint8_t, 8> buffer{};
    QCOMPARE(transport.read(buffer.data(), static_cast<int>(buffer.size()), 0).status, GPSReadStatus::TimedOut);
}

void UDPGPSTransportTest::_cancelledRead()
{
    std::atomic_bool stop = false;
    UDPGPSTransport transport(0, stop);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    stop = true;
    std::array<uint8_t, 8> buffer{};
    QCOMPARE(transport.read(buffer.data(), static_cast<int>(buffer.size()), 1000).status, GPSReadStatus::Cancelled);
    UDPGPSTransport unopened(0, stop);
    QCOMPARE(unopened.open().status, GPSOpenStatus::Cancelled);
}

void UDPGPSTransportTest::_portInUse()
{
    QUdpSocket owner;
    QVERIFY(owner.bind(QHostAddress::AnyIPv4, 0));
    std::atomic_bool stop = false;
    UDPGPSTransport transport(owner.localPort(), stop);
    expectLogMessage("GPS.Transport.UDPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Cannot listen for receiver data on UDP port")));
    const auto opened = transport.open();
    verifyExpectedLogMessage();
    QCOMPARE(opened.status, GPSOpenStatus::Error);
    QVERIFY(!opened.detail.isEmpty());
    QVERIFY(transport.fatalError());
}

UT_REGISTER_TEST(UDPGPSTransportTest, TestLabel::Unit)
