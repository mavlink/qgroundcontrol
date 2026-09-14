#include "UdpForwarderTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QTest>

#include "UdpForwarder.h"

void UdpForwarderTest::testInitialState()
{
    UdpForwarder fwd;
    QVERIFY(!fwd.isEnabled());
    QCOMPARE(fwd.forward(QByteArrayLiteral("disabled")), qint64(0));
    QCOMPARE(fwd.port(), quint16(0));
    QCOMPARE(fwd.address(), QString());
}

void UdpForwarderTest::testConfigureValid()
{
    UdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(fwd.isEnabled());
    QCOMPARE(fwd.address(), QStringLiteral("127.0.0.1"));
    QCOMPARE(fwd.port(), quint16(9000));
}

void UdpForwarderTest::testConfigureInvalid()
{
    UdpForwarder fwd;

    expectLogMessage("Utilities.UdpForwarder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid UDP forward config")));
    QVERIFY(!fwd.configure(QString(), 9000));
    verifyExpectedLogMessage();
    QVERIFY(!fwd.isEnabled());

    expectLogMessage("Utilities.UdpForwarder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid UDP forward config")));
    QVERIFY(!fwd.configure(QStringLiteral("127.0.0.1"), 0));
    verifyExpectedLogMessage();
    QVERIFY(!fwd.isEnabled());
}

void UdpForwarderTest::testForward()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    const quint16 port = receiver.localPort();

    UdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), port));

    const QByteArray payload = QByteArrayLiteral("test-rtcm-data");
    QCOMPARE(fwd.forward(payload), qint64(payload.size()));

    QVERIFY(receiver.waitForReadyRead(1000));
    QByteArray received;
    received.resize(receiver.pendingDatagramSize());
    receiver.readDatagram(received.data(), received.size());
    QCOMPARE(received, payload);
}

void UdpForwarderTest::testStop()
{
    UdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(fwd.isEnabled());

    fwd.stop();
    QVERIFY(!fwd.isEnabled());
    QCOMPARE(fwd.forward(QByteArrayLiteral("disabled")), qint64(0));
    QCOMPARE(fwd.port(), quint16(0));
}

void UdpForwarderTest::testReconfigure_data()
{
    QTest::addColumn<QString>("firstAddress");
    QTest::addColumn<QString>("secondAddress");
    QTest::newRow("ipv4") << QStringLiteral("127.0.0.1") << QStringLiteral("127.0.0.1");
    QTest::newRow("ipv4-to-ipv6") << QStringLiteral("127.0.0.1") << QStringLiteral("::1");
    QTest::newRow("ipv6-to-ipv4") << QStringLiteral("::1") << QStringLiteral("127.0.0.1");
}

void UdpForwarderTest::testReconfigure()
{
    QFETCH(QString, firstAddress);
    QFETCH(QString, secondAddress);
    QUdpSocket firstReceiver;
    QUdpSocket secondReceiver;
    const auto bindReceiver = [](QUdpSocket& socket, const QString& address) {
        return socket.bind(QHostAddress(address), 0);
    };
    if (!bindReceiver(firstReceiver, firstAddress) || !bindReceiver(secondReceiver, secondAddress)) {
        if (firstAddress == QStringLiteral("::1") || secondAddress == QStringLiteral("::1")) {
            QSKIP("IPv6 loopback is unavailable");
        }
        QFAIL("Could not bind IPv4 loopback receivers");
    }
    UdpForwarder fwd;
    QVERIFY(fwd.configure(firstAddress, firstReceiver.localPort()));
    QCOMPARE(fwd.forward(QByteArrayLiteral("first")), 5);
    QTRY_VERIFY_WITH_TIMEOUT(firstReceiver.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(firstReceiver.receiveDatagram().data(), QByteArrayLiteral("first"));

    QVERIFY(fwd.configure(secondAddress, secondReceiver.localPort()));
    QCOMPARE(fwd.port(), secondReceiver.localPort());
    QCOMPARE(fwd.forward(QByteArrayLiteral("second")), 6);
    QTRY_VERIFY_WITH_TIMEOUT(secondReceiver.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(secondReceiver.receiveDatagram().data(), QByteArrayLiteral("second"));

    fwd.stop();
    QVERIFY(!fwd.isEnabled());
    QCOMPARE(fwd.forward(QByteArrayLiteral("disabled")), 0);
    QVERIFY(fwd.configure(firstAddress, firstReceiver.localPort()));
    QCOMPARE(fwd.forward(QByteArrayLiteral("restarted")), 9);
    QTRY_VERIFY_WITH_TIMEOUT(firstReceiver.hasPendingDatagrams(), TestTimeout::mediumMs());
    QCOMPARE(firstReceiver.receiveDatagram().data(), QByteArrayLiteral("restarted"));
}

UT_REGISTER_TEST(UdpForwarderTest, TestLabel::Unit)
