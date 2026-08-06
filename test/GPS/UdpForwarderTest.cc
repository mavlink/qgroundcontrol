#include "UdpForwarderTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QTest>

#include "UdpForwarder.h"

void UdpForwarderTest::testInitialState()
{
    UdpForwarder fwd;
    QVERIFY(!fwd.isEnabled());
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

    expectLogMessage("GPS.UdpForwarder", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid UDP forward config")));
    QVERIFY(!fwd.configure(QString(), 9000));
    verifyExpectedLogMessage();
    QVERIFY(!fwd.isEnabled());

    expectLogMessage("GPS.UdpForwarder", QtWarningMsg,
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
    fwd.forward(payload);

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
    QCOMPARE(fwd.port(), quint16(0));
}

void UdpForwarderTest::testReconfigure()
{
    UdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9001));
    QCOMPARE(fwd.port(), quint16(9001));
    QVERIFY(fwd.isEnabled());
}

UT_REGISTER_TEST(UdpForwarderTest, TestLabel::Unit)
