#include "RTCMUdpInputTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QThread>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCMUdpInput.h"

namespace {

/// The port of @a input's bound socket, or 0 while it holds none.
quint16 boundPort(const RTCMUdpInput& input)
{
    for (const auto* socket : input.findChildren<QUdpSocket*>()) {
        if (socket->state() == QAbstractSocket::BoundState) {
            return socket->localPort();
        }
    }
    return 0;
}

/// The frame data of one recorded RTCMUdpInput::frameReceived() emission.
QByteArray frameData(const QList<QVariant>& emission)
{
    return emission.at(1).toByteArray();
}

bool sendDatagram(quint16 port, const QByteArray& payload)
{
    QUdpSocket sender;
    return sender.writeDatagram(payload, QHostAddress::LocalHost, port) == payload.size();
}

// Loopback delivery is asynchronous on some platforms (macOS); wait without dispatching readyRead.
bool waitForPendingDatagram(const RTCMUdpInput& input)
{
    const auto* socket = input.findChild<QUdpSocket*>();
    if (!socket) {
        return false;
    }
    const QDeadlineTimer deadline(TestTimeout::shortMs());
    while (!socket->hasPendingDatagrams()) {
        if (deadline.hasExpired()) {
            return false;
        }
        QThread::yieldCurrentThread();
    }
    return true;
}

}  // namespace

void RTCMUdpInputTest::_testSocketErrors_data()
{
    QTest::addColumn<bool>("restart");
    QTest::newRow("retired-after-stop") << false;
    QTest::newRow("retired-after-restart") << true;
}

void RTCMUdpInputTest::_testSocketErrors()
{
    QFETCH(bool, restart);
    RTCMUdpInput input;
    QVERIFY(input.start(0));
    const QPointer<QUdpSocket> socket = input.findChild<QUdpSocket*>();
    QVERIFY(socket);
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QSignalSpy reads(socket, &QUdpSocket::readyRead);
    QUdpSocket sender;
    const auto frame = GPSTest::rtcmMessage(1005, 20);
    const auto prefix = frame.first(5);
    QCOMPARE(sender.writeDatagram(prefix, QHostAddress::LocalHost, boundPort(input)), prefix.size());
    QTRY_VERIFY_WITH_TIMEOUT(!reads.isEmpty(), TestTimeout::mediumMs());
    QVERIFY(frames.isEmpty());
    expectLogMessage("GPS.Corrections.RTCMUdpInput", QtWarningMsg,
                     QRegularExpression(QStringLiteral("UDP socket error on port %1.*%2")
                                            .arg(boundPort(input))
                                            .arg(QRegularExpression::escape(socket->errorString()))));
    QVERIFY(QMetaObject::invokeMethod(socket, "errorOccurred", Qt::DirectConnection,
                                      Q_ARG(QAbstractSocket::SocketError, QAbstractSocket::NetworkError)));
    verifyExpectedLogMessage();
    QVERIFY(boundPort(input) != 0);
    QCOMPARE(socket->state(), QAbstractSocket::BoundState);
    QVERIFY(frames.isEmpty());
    const auto tail = frame.sliced(5);
    QCOMPARE(sender.writeDatagram(tail, QHostAddress::LocalHost, boundPort(input)), tail.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(frameData(frames.first()), frame);
    frames.clear();

    QSignalSpy errors(socket, &QUdpSocket::errorOccurred);
    QVERIFY(QMetaObject::invokeMethod(
        socket, [socket]() { emit socket->errorOccurred(QAbstractSocket::TemporaryError); }, Qt::QueuedConnection));
    input.stop();
    if (restart) {
        QVERIFY(input.start(0));
    }
    QVERIFY(socket);
    // Strict log checking fails the test if the retired socket's error is logged.
    QCoreApplication::sendPostedEvents(socket, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(boundPort(input) != 0, restart);
    QVERIFY(frames.isEmpty());
    if (restart) {
        QVERIFY(sendDatagram(boundPort(input), frame));
        QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    }
}

void RTCMUdpInputTest::_testSenderInstanceNaming_data()
{
    QTest::addColumn<QHostAddress>("sender");
    QTest::newRow("ipv4") << QHostAddress(QHostAddress::LocalHost);
    QTest::newRow("ipv6") << QHostAddress(QHostAddress::LocalHostIPv6);
}

void RTCMUdpInputTest::_testSenderInstanceNaming()
{
    QFETCH(QHostAddress, sender);
    RTCMUdpInput input;
    QVERIFY(input.start(0));
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);
    QUdpSocket socket;
    if (!socket.bind(sender, 0)) {
        QSKIP("This host has no loopback address of this family");
    }

    const QByteArray payload = GPSTest::rtcmMessage(1005, 20);
    QCOMPARE(socket.writeDatagram(payload, sender, boundPort(input)), payload.size());

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(frameData(spy.at(0)), payload);
    // The dual-stack input names IPv4 senders by their IPv4 address, not the IPv4-mapped form.
    QCOMPARE(spy.at(0).at(0).toString(), sender.toString());
}

void RTCMUdpInputTest::_testEmitsOneSignalPerFrame()
{
    RTCMUdpInput input;
    QVERIFY(input.start(0));
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);

    // Each valid frame of a datagram is emitted separately so RTCMMAVLink assigns it its own sequence; garbage and
    // a corrupt frame between them are dropped. How the framer recovers is RTCMConformanceTest's.
    const QByteArray frame1 = GPSTest::rtcmMessage(1005, 4);
    QByteArray corrupted = GPSTest::rtcmMessage(1077, 200);
    corrupted[corrupted.size() - 1] = static_cast<char>(corrupted[corrupted.size() - 1] ^ 0xFF);
    const QByteArray frame2 = GPSTest::rtcmMessage(1087, 2);
    const QByteArray garbage = QByteArrayLiteral("\x01\x02\x03");
    QVERIFY(sendDatagram(boundPort(input), garbage + frame1 + corrupted + frame2));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, TestTimeout::mediumMs());
    QCOMPARE(frameData(spy.at(0)), frame1);
    QCOMPARE(frameData(spy.at(1)), frame2);
    // Frames of one datagram share its sender and receipt time.
    QCOMPARE(spy.at(0).at(0), spy.at(1).at(0));
    QCOMPARE(spy.at(0).at(2), spy.at(1).at(2));
}

void RTCMUdpInputTest::_testFrameSplitAcrossDatagrams()
{
    RTCMUdpInput input;
    QVERIFY(input.start(0));
    QSignalSpy spy(&input, &RTCMUdpInput::frameReceived);

    // Parser state must carry across datagrams so a frame split by the sender
    // still comes out whole.
    const QByteArray frame = GPSTest::rtcmMessage(1005, 6);
    const int split = frame.size() / 2;
    QUdpSocket sender;
    QCOMPARE(sender.writeDatagram(frame.left(split), QHostAddress::LocalHost, boundPort(input)), split);
    QCOMPARE(sender.writeDatagram(frame.mid(split), QHostAddress::LocalHost, boundPort(input)), frame.size() - split);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(frameData(spy.at(0)), frame);
}

void RTCMUdpInputTest::_testInterleavedSenders()
{
    RTCMUdpInput input;
    QVERIFY(input.start(0));
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QUdpSocket senderA;
    QUdpSocket senderB;
    const QByteArray frameA = GPSTest::rtcmMessage(1005, 20);
    const QByteArray frameB = GPSTest::rtcmMessage(1077, 40);
    const int split = 5;
    QCOMPARE(senderA.writeDatagram(frameA.first(split), QHostAddress::LocalHost, boundPort(input)), split);
    QCOMPARE(senderB.writeDatagram(frameB, QHostAddress::LocalHost, boundPort(input)), frameB.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.count(), 1, TestTimeout::mediumMs());
    QCOMPARE(frameData(frames.first()), frameB);
    QCOMPARE(senderA.writeDatagram(frameA.sliced(split), QHostAddress::LocalHost, boundPort(input)),
             frameA.size() - split);
    QTRY_COMPARE_WITH_TIMEOUT(frames.count(), 2, TestTimeout::mediumMs());
    QCOMPARE(frameData(frames.last()), frameA);
    // Source ports frame independently but name one stream, so selection survives a sender restart on a new port.
    QCOMPARE(frames.last().at(0).toString(), QHostAddress(QHostAddress::LocalHost).toString());
    QCOMPARE(frames.first().at(0), frames.last().at(0));
    // A frame is stamped when its first bytes arrived.
    QVERIFY(frames.last().at(2).toLongLong() <= frames.first().at(2).toLongLong());
}

void RTCMUdpInputTest::_testBurstYieldsBetweenDrains()
{
    RTCMUdpInput input;
    QVERIFY(input.start(0));
    QSignalSpy frames(&input, &RTCMUdpInput::frameReceived);
    QUdpSocket sender;
    const QByteArray payload = GPSTest::rtcmMessage(1005, 20);
    for (int i = 0; i < 40; ++i) {
        QCOMPARE(sender.writeDatagram(payload, QHostAddress::LocalHost, boundPort(input)), payload.size());
    }
    // Invoke one drain without processing the event loop: it must leave work for a continuation.
    QVERIFY(waitForPendingDatagram(input));
    QVERIFY(QMetaObject::invokeMethod(&input, "_readDatagrams", Qt::DirectConnection));
    QVERIFY(frames.size() > 0);
    QVERIFY(frames.size() <= 16);
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 40, TestTimeout::mediumMs());
    for (const auto& received : frames) {
        QCOMPARE(frameData(received), payload);
    }
}

UT_REGISTER_TEST(RTCMUdpInputTest, TestLabel::Unit)
