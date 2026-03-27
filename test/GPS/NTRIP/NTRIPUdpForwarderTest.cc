#include "NTRIPUdpForwarderTest.h"
#include "NTRIPUdpForwarder.h"

#include <QtNetwork/QUdpSocket>
#include <QtTest/QTest>

void NTRIPUdpForwarderTest::testInitialState()
{
    NTRIPUdpForwarder fwd;
    QVERIFY(!fwd.isEnabled());
    QCOMPARE(fwd.port(), quint16(0));
    QCOMPARE(fwd.address(), QString());
}

void NTRIPUdpForwarderTest::testConfigureValid()
{
    NTRIPUdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(fwd.isEnabled());
    QCOMPARE(fwd.address(), QStringLiteral("127.0.0.1"));
    QCOMPARE(fwd.port(), quint16(9000));
}

void NTRIPUdpForwarderTest::testConfigureInvalid()
{
    NTRIPUdpForwarder fwd;
    QVERIFY(!fwd.configure(QString(), 9000));
    QVERIFY(!fwd.isEnabled());

    QVERIFY(!fwd.configure(QStringLiteral("127.0.0.1"), 0));
    QVERIFY(!fwd.isEnabled());
}

void NTRIPUdpForwarderTest::testForward()
{
    QUdpSocket receiver;
    QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
    const quint16 port = receiver.localPort();

    NTRIPUdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), port));

    const QByteArray payload = QByteArrayLiteral("test-rtcm-data");
    fwd.forward(payload);

    QVERIFY(receiver.waitForReadyRead(1000));
    QByteArray received;
    received.resize(receiver.pendingDatagramSize());
    receiver.readDatagram(received.data(), received.size());
    QCOMPARE(received, payload);
}

void NTRIPUdpForwarderTest::testStop()
{
    NTRIPUdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(fwd.isEnabled());

    fwd.stop();
    QVERIFY(!fwd.isEnabled());
    QCOMPARE(fwd.port(), quint16(0));
}

void NTRIPUdpForwarderTest::testReconfigure()
{
    NTRIPUdpForwarder fwd;
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9000));
    QVERIFY(fwd.configure(QStringLiteral("127.0.0.1"), 9001));
    QCOMPARE(fwd.port(), quint16(9001));
    QVERIFY(fwd.isEnabled());
}

UT_REGISTER_TEST(NTRIPUdpForwarderTest, TestLabel::Unit)
