#include "UdpGPSTransportTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "UdpGPSTransport.h"

void UdpGPSTransportTest::_transferAndPartialReads_data()
{
    QTest::addColumn<QString>("host");
    QTest::newRow("ipv4") << QStringLiteral("127.0.0.1");
    QTest::newRow("ipv6") << QStringLiteral("::1");
    QTest::newRow("hostname") << QStringLiteral("localhost");
}

void UdpGPSTransportTest::_transferAndPartialReads()
{
    QFETCH(QString, host);
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::Any, 0));
    QUdpSocket localPortProbe;
    QVERIFY(localPortProbe.bind(QHostAddress::Any, 0));
    const quint16 localPort = localPortProbe.localPort();
    localPortProbe.close();

    std::atomic_bool stop = false;
    UdpGPSTransport transport(host, receiver.localPort(), stop, localPort);
    QVERIFY(transport.open());
    QVERIFY(!transport.fatalError());
    QCOMPARE(transport.fixedBaudrate(), 115200u);
    QVERIFY(transport.setBaudrate(115200));
    QVERIFY(!transport.setBaudrate(9600));

    const QByteArray payload = QByteArray::fromHex("b56201020300d300ff");
    QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()), payload.size());
    QTRY_VERIFY_WITH_TIMEOUT(receiver.hasPendingDatagrams(), TestTimeout::shortMs());
    const QNetworkDatagram request = receiver.receiveDatagram();
    QCOMPARE(request.data(), payload);
    QCOMPARE(request.senderPort(), localPort);

    QUdpSocket unrelated;
    QCOMPARE(unrelated.writeDatagram("wrong peer", request.senderAddress(), localPort), 10);
    uint8_t buffer[64]{};
    QCOMPARE(transport.read(buffer, sizeof(buffer), 20), 0);
    QVERIFY(!transport.fatalError());

    QCOMPARE(receiver.writeDatagram(QByteArray(), request.senderAddress(), localPort), 0);
    QCOMPARE(receiver.writeDatagram(payload, request.senderAddress(), localPort), payload.size());
    QCOMPARE(receiver.writeDatagram("next", request.senderAddress(), localPort), 4);
    QCOMPARE(transport.read(buffer, 3, TestTimeout::shortMs()), 3);
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), 3), payload.left(3));
    QCOMPARE(transport.read(buffer, sizeof(buffer), 0), payload.size() - 3);
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), payload.size() - 3), payload.mid(3));
    QCOMPARE(transport.read(buffer, sizeof(buffer), TestTimeout::shortMs()), 4);
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), 4), QByteArray("next"));
    QCOMPARE(transport.read(buffer, sizeof(buffer), 20), 0);
    QVERIFY(!transport.fatalError());

    stop = true;
    QCOMPARE(transport.read(buffer, sizeof(buffer), TestTimeout::shortMs()), -1);
    QCOMPARE(transport.write(buffer, 1), -1);
}

void UdpGPSTransportTest::_cancelRead()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    std::atomic_bool stop = true;
    UdpGPSTransport transport(QStringLiteral("127.0.0.1"), receiver.localPort(), stop);
    QVERIFY(!transport.open());
    stop = false;
    QVERIFY(transport.open());
    QTimer::singleShot(0, &receiver, [&]() { stop = true; });
    QElapsedTimer elapsed;
    elapsed.start();
    uint8_t buffer[8]{};
    QCOMPARE(transport.read(buffer, sizeof(buffer), TestTimeout::longMs()), -1);
    QVERIFY(stop);
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
}

void UdpGPSTransportTest::_bindFailure()
{
    QUdpSocket occupied;
    QVERIFY(occupied.bind(QHostAddress::Any, 0, QAbstractSocket::DontShareAddress));
    std::atomic_bool stop = false;
    UdpGPSTransport transport(QStringLiteral("127.0.0.1"), 2101, stop, occupied.localPort());
    expectLogMessage("GPS.RTK.UdpGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to open UDP GPS receiver")));
    QVERIFY(!transport.open());
    verifyExpectedLogMessage();
    QVERIFY(transport.fatalError());
    uint8_t byte{};
    QCOMPARE(transport.read(&byte, 1, 0), -1);
    QCOMPARE(transport.write(&byte, 1), -1);
}

UT_REGISTER_TEST(UdpGPSTransportTest, TestLabel::Unit)
