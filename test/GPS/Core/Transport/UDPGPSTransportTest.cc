#include "UDPGPSTransportTest.h"

#include <array>
#include <chrono>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtNetwork/QUdpSocket>

#include "GPSCancellation.h"
#include "Transport/Support/UnusedUdpPort.h"
#include "UDPGPSTransport.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// One poll of the datagrams the test's sockets just sent; the QTRY loops around it bound the wait.
constexpr std::chrono::milliseconds POLL{20};

QByteArray readAll(UDPGPSTransport& transport, std::chrono::milliseconds timeout)
{
    std::array<uint8_t, 256> buffer{};
    QByteArray received;
    for (;;) {
        const auto result = transport.read(buffer, timeout);
        if (result.status != GPSReadStatus::Data || result.bytesRead == 0) {
            return received;
        }
        received.append(reinterpret_cast<const char*>(buffer.data()), result.bytesRead);
        timeout = 0ms;
    }
}
}  // namespace

std::unique_ptr<UDPGPSTransport> UDPGPSTransportTest::_openUnusedPort(quint16& port, GPSCancelToken cancelToken,
                                                                      std::chrono::milliseconds peerIdleTimeout)
{
    std::unique_ptr<UDPGPSTransport> transport;
    int failures = 0;
    port = bindUnusedUdpPort(
        [&](quint16 candidate) {
            transport = std::make_unique<UDPGPSTransport>(candidate, cancelToken, peerIdleTimeout);
            return transport->open().status == GPSOpenStatus::Opened;
        },
        failures);
    if (failures > 0) {
        // Another process took a probed port before the transport bound it; the next port was tried.
        ignoreLogMessage("GPS.Transport.UDPGPSTransport", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Cannot listen for receiver data on UDP port")));
    }
    return port != 0 ? std::move(transport) : nullptr;
}

void UDPGPSTransportTest::_receivesSelectedSender()
{
    GPSCancelSource stop;
    quint16 port = 0;
    const auto opened = _openUnusedPort(port, stop.token());
    QVERIFY(opened);
    UDPGPSTransport& transport = *opened;
    QVERIFY(!transport.fatalError());
    QUdpSocket first;
    QUdpSocket second;
    QVERIFY(first.bind(QHostAddress::LocalHost, 0));
    QVERIFY(second.bind(QHostAddress::LocalHost, 0));
    QCOMPARE(first.writeDatagram("$GPGGA,1*", QHostAddress::LocalHost, port), 9);
    QCOMPARE(second.writeDatagram("ignored", QHostAddress::LocalHost, port), 7);
    QCOMPARE(first.writeDatagram("00\r\n", QHostAddress::LocalHost, port), 4);
    QByteArray received;
    QTRY_COMPARE_WITH_TIMEOUT((received += readAll(transport, POLL), received), QByteArray("$GPGGA,1*00\r\n"),
                              TestTimeout::mediumMs());
    QCOMPARE(readAll(transport, 0ms), QByteArray());
}

void UDPGPSTransportTest::_receivesIPv6Sender()
{
    QUdpSocket sender;
    if (!sender.bind(QHostAddress::LocalHostIPv6, 0)) {
        QSKIP("IPv6 loopback is unavailable");
    }
    GPSCancelSource stop;
    quint16 port = 0;
    const auto opened = _openUnusedPort(port, stop.token());
    QVERIFY(opened);
    UDPGPSTransport& transport = *opened;
    QCOMPARE(sender.writeDatagram("$GPGGA,6*00\r\n", QHostAddress::LocalHostIPv6, port), 13);
    QByteArray received;
    QTRY_COMPARE_WITH_TIMEOUT((received += readAll(transport, POLL), received), QByteArray("$GPGGA,6*00\r\n"),
                              TestTimeout::mediumMs());
}

void UDPGPSTransportTest::_idleSenderIsReplaced()
{
    GPSCancelSource stop;
    quint16 port = 0;
    // A short idle timeout, so the test need not wait the default five seconds for a sender to go idle.
    const auto opened = _openUnusedPort(port, stop.token(), 50ms);
    QVERIFY(opened);
    UDPGPSTransport& transport = *opened;
    QUdpSocket first;
    QUdpSocket second;
    QVERIFY(first.bind(QHostAddress::LocalHost, 0));
    QVERIFY(second.bind(QHostAddress::LocalHost, 0));
    QCOMPARE(first.writeDatagram("$partial", QHostAddress::LocalHost, port), 8);
    QByteArray received;
    QTRY_COMPARE_WITH_TIMEOUT((received += readAll(transport, POLL), received), QByteArray("$partial"),
                              TestTimeout::mediumMs());
    // Once the selected sender is idle, another sender takes over without inheriting its bytes.
    QTRY_VERIFY_WITH_TIMEOUT(
        second.writeDatagram("$next", QHostAddress::LocalHost, port) == 5 && readAll(transport, POLL).endsWith("$next"),
        TestTimeout::mediumMs());
}

void UDPGPSTransportTest::_receiveOnlyLink()
{
    GPSCancelSource stop;
    UDPGPSTransport transport(0, stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QCOMPARE(transport.fixedBaudrate(), GPSTransport::BRIDGE_BAUDRATE);
    QVERIFY(transport.setBaudrate(GPSTransport::BRIDGE_BAUDRATE));
    QVERIFY(!transport.setBaudrate(9600));
    const auto written = transport.write("$P", QDeadlineTimer(TestTimeout::shortMs()));
    QCOMPARE(written.status, GPSWriteStatus::Unsupported);
    QCOMPARE(written.acceptedBytes, 0);
    std::array<uint8_t, 8> buffer{};
    QCOMPARE(transport.read(buffer, 0ms).status, GPSReadStatus::TimedOut);
}

void UDPGPSTransportTest::_cancelledRead()
{
    GPSCancelSource stop;
    UDPGPSTransport transport(0, stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    stop.cancel();
    std::array<uint8_t, 8> buffer{};
    // Cancelled before it waits, so the timeout is never spent.
    QCOMPARE(transport.read(buffer, 1000ms).status, GPSReadStatus::Cancelled);
    UDPGPSTransport unopened(0, stop.token());
    QCOMPARE(unopened.open().status, GPSOpenStatus::Cancelled);
}

void UDPGPSTransportTest::_portInUse()
{
    QUdpSocket owner;
    // Match the transport's dual-stack bind: BSD stacks (macOS) let it coexist with an IPv4-only owner.
    QVERIFY(owner.bind(QHostAddress::Any, 0));
    GPSCancelSource stop;
    UDPGPSTransport transport(owner.localPort(), stop.token());
    expectLogMessage("GPS.Transport.UDPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Cannot listen for receiver data on UDP port")));
    const auto opened = transport.open();
    verifyExpectedLogMessage();
    QCOMPARE(opened.status, GPSOpenStatus::Error);
    QVERIFY(!opened.detail.isEmpty());
    QVERIFY(transport.fatalError());
}

UT_REGISTER_TEST_LIGHTWEIGHT(UDPGPSTransportTest, TestLabel::Unit)
