#include "RTCMUdpInputTest.h"

#include <QtCore/QRegularExpression>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GpsTestHelpers.h"
#include "RTCMUdpInput.h"

namespace {

bool sendDatagram(quint16 port, const QByteArray& payload)
{
    QUdpSocket sender;
    return sender.writeDatagram(payload, QHostAddress::LocalHost, port) == payload.size();
}

}  // namespace

void RTCMUdpInputTest::_testStartStop()
{
    RTCMUdpInput input(0);
    QVERIFY(input.start());
    QVERIFY(input.isRunning());
    QVERIFY(input.port() != 0);  // ephemeral port resolved on bind

    input.stop();
    QVERIFY(!input.isRunning());
}

void RTCMUdpInputTest::_testPassthroughWithoutValidation()
{
    RTCMUdpInput input(0);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::rtcmDataReceived);

    // Validation off (default): datagram forwarded as-is, valid RTCM or not.
    const QByteArray payload = QByteArrayLiteral("not-rtcm-at-all");
    QVERIFY(sendDatagram(input.port(), payload));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QCOMPARE(spy.at(0).at(0).toByteArray(), payload);
}

void RTCMUdpInputTest::_testEmitsOneSignalPerFrame()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::rtcmDataReceived);

    // One datagram carrying two frames plus leading garbage: each frame must be
    // emitted separately so RTCMMavlink assigns it its own sequence.
    const QByteArray frame1 = GpsTestHelpers::buildRtcmFrame(1005, 4);
    const QByteArray frame2 = GpsTestHelpers::buildRtcmFrame(1077, 200);
    const QByteArray garbage = QByteArrayLiteral("\x01\x02\x03");
    QVERIFY(sendDatagram(input.port(), garbage + frame1 + frame2));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 2000);
    QCOMPARE(spy.at(0).at(0).toByteArray(), frame1);
    QCOMPARE(spy.at(1).at(0).toByteArray(), frame2);
}

void RTCMUdpInputTest::_testDropsBadCrcFrame()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::rtcmDataReceived);

    const QByteArray frame1 = GpsTestHelpers::buildRtcmFrame(1005, 4);
    QByteArray corrupted = GpsTestHelpers::buildRtcmFrame(1077, 8);
    corrupted[corrupted.size() - 1] = static_cast<char>(corrupted[corrupted.size() - 1] ^ 0xFF);
    const QByteArray frame2 = GpsTestHelpers::buildRtcmFrame(1087, 2);

    expectLogMessage("GPS.RTCMUdpInput", QtWarningMsg, QRegularExpression(QStringLiteral("Dropped 1 RTCM frame")));
    QVERIFY(sendDatagram(input.port(), frame1 + corrupted + frame2));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 2000);
    verifyExpectedLogMessage();
    QCOMPARE(spy.at(0).at(0).toByteArray(), frame1);
    QCOMPARE(spy.at(1).at(0).toByteArray(), frame2);
}

void RTCMUdpInputTest::_testFrameSplitAcrossDatagrams()
{
    RTCMUdpInput input(0);
    input.setValidation(true);
    QVERIFY(input.start());
    QSignalSpy spy(&input, &RTCMUdpInput::rtcmDataReceived);

    // Parser state must carry across datagrams so a frame split by the sender
    // still comes out whole.
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 6);
    const int split = frame.size() / 2;
    QVERIFY(sendDatagram(input.port(), frame.left(split)));
    QVERIFY(sendDatagram(input.port(), frame.mid(split)));

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QCOMPARE(spy.at(0).at(0).toByteArray(), frame);
}

UT_REGISTER_TEST(RTCMUdpInputTest, TestLabel::Unit)
