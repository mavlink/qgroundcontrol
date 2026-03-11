#include "GpsTest.h"

#include <QtCore/QByteArray>
#include <QtTest/QSignalSpy>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>

#include "NTRIP.h"

namespace {

QByteArray _buildRtcmFrame(uint16_t messageId, const QByteArray &extraPayload = {}, const QByteArray &crc = QByteArray::fromHex("010203"))
{
    QByteArray payload;
    payload.append(static_cast<char>((messageId >> 4) & 0xFF));
    payload.append(static_cast<char>((messageId & 0x0F) << 4));
    payload.append(extraPayload);

    const uint16_t messageLength = static_cast<uint16_t>(payload.size());

    QByteArray frame;
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>((messageLength >> 8) & 0x03));
    frame.append(static_cast<char>(messageLength & 0xFF));
    frame.append(payload);
    frame.append(crc);

    return frame;
}

} // namespace

void GpsTest::_testGpsRTCM()
{
    RTCMParser parser;
    const QByteArray frame = _buildRtcmFrame(1005, QByteArray::fromHex("AABB"));
    QVERIFY(frame.size() > 6);

    for (int i = 0; i < frame.size(); i++) {
        const bool complete = parser.addByte(static_cast<uint8_t>(frame.at(i)));
        if (i < (frame.size() - 1)) {
            QVERIFY(!complete);
        } else {
            QVERIFY(complete);
        }
    }

    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(4));
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1005));
    QCOMPARE(parser.message()[0], static_cast<uint8_t>(RTCM3_PREAMBLE));
    QCOMPARE(parser.crcBytes()[0], static_cast<uint8_t>(0x01));
    QCOMPARE(parser.crcBytes()[1], static_cast<uint8_t>(0x02));
    QCOMPARE(parser.crcBytes()[2], static_cast<uint8_t>(0x03));
}

void GpsTest::_testRtcmParserIgnoresBytesUntilPreamble()
{
    RTCMParser parser;

    const QByteArray noise = QByteArray::fromHex("0001027FFF");
    for (const char byte : noise) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(byte)));
    }

    const QByteArray frame = _buildRtcmFrame(1074);
    bool complete = false;
    for (const char byte : frame) {
        complete = parser.addByte(static_cast<uint8_t>(byte));
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1074));
}

void GpsTest::_testRtcmParserRecoversAfterInvalidLength()
{
    RTCMParser parser;

    // Invalid length (0) should force parser reset.
    QVERIFY(!parser.addByte(RTCM3_PREAMBLE));
    QVERIFY(!parser.addByte(0x00));
    QVERIFY(!parser.addByte(0x00));

    const QByteArray frame = _buildRtcmFrame(1033, QByteArray::fromHex("11"));
    bool complete = false;
    for (const char byte : frame) {
        complete = parser.addByte(static_cast<uint8_t>(byte));
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1033));
    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(3));
}

void GpsTest::_testRtcmParserRejectsOversizedLengthAndRecovers()
{
    RTCMParser parser;

    // Oversized length (1021) should be rejected and force parser reset.
    QVERIFY(!parser.addByte(RTCM3_PREAMBLE));
    QVERIFY(!parser.addByte(0x03));
    QVERIFY(!parser.addByte(0xFD));

    const QByteArray frame = _buildRtcmFrame(1042, QByteArray::fromHex("ABCD"));
    bool complete = false;
    for (const char byte : frame) {
        complete = parser.addByte(static_cast<uint8_t>(byte));
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1042));
    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(4));
}

void GpsTest::_testRtcmParserResetClearsPartialState()
{
    RTCMParser parser;
    const QByteArray frame = _buildRtcmFrame(1019, QByteArray::fromHex("22"));

    // Feed partial frame then reset.
    QVERIFY(!parser.addByte(static_cast<uint8_t>(frame.at(0))));
    QVERIFY(!parser.addByte(static_cast<uint8_t>(frame.at(1))));
    QVERIFY(!parser.addByte(static_cast<uint8_t>(frame.at(2))));
    QVERIFY(!parser.addByte(static_cast<uint8_t>(frame.at(3))));
    parser.reset();

    // Remaining bytes from the old frame must not complete a packet.
    for (int i = 4; i < frame.size(); i++) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(frame.at(i))));
    }

    // A complete new frame should parse correctly after reset.
    bool complete = false;
    for (const char byte : frame) {
        complete = parser.addByte(static_cast<uint8_t>(byte));
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1019));
}

void GpsTest::_testNtripAutoStartDisabledDefersSocketCreation()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    const int port = server.serverPort();

    NTRIPTCPLink link(QStringLiteral("127.0.0.1"), port, QString(), QString(), QString(), QString(), false, false);

    QVERIFY(link._rtcmParser != nullptr);
    QVERIFY(link._socket == nullptr);

    link.start();
    QVERIFY(link._socket != nullptr);
}

void GpsTest::_testNtripAutoStartDefaultCreatesSocket()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    const int port = server.serverPort();

    NTRIPTCPLink link(QStringLiteral("127.0.0.1"), port, QString(), QString(), QString(), QString(), false);

    QVERIFY(link._rtcmParser != nullptr);
    QVERIFY(link._socket != nullptr);
}

void GpsTest::_testNtripWhitelistFiltersMessages()
{
    NTRIPTCPLink link(QStringLiteral("127.0.0.1"), 2101, QString(), QString(), QString(), QStringLiteral("1005"),
                      false, false);
    QSignalSpy rtcmSpy(&link, &NTRIPTCPLink::RTCMDataUpdate);
    QVERIFY(rtcmSpy.isValid());

    const QByteArray blockedFrame = _buildRtcmFrame(1074, QByteArray::fromHex("AA"));
    const QByteArray allowedFrame = _buildRtcmFrame(1005, QByteArray::fromHex("BB"));
    link._parse(blockedFrame + allowedFrame + blockedFrame);

    QCOMPARE(rtcmSpy.count(), 1);
    const QByteArray forwarded = rtcmSpy.takeFirst().at(0).toByteArray();
    QVERIFY(!forwarded.isEmpty());

    RTCMParser parser;
    bool complete = false;
    for (const char byte : forwarded) {
        complete = parser.addByte(static_cast<uint8_t>(byte));
    }
    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1005));
}

void GpsTest::_testNtripSpartnHeaderIsStrippedOnlyOnce()
{
    NTRIPTCPLink link(QStringLiteral("127.0.0.1"), 2102, QString(), QString(), QString(), QString(), true, false);
    QSignalSpy spartnSpy(&link, &NTRIPTCPLink::SPARTNDataUpdate);
    QVERIFY(spartnSpy.isValid());

    link._handleSpartnData("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n");
    QCOMPARE(spartnSpy.count(), 0);

    link._handleSpartnData("\r\nPAYLOAD1");
    QCOMPARE(spartnSpy.count(), 1);
    QCOMPARE(spartnSpy.takeFirst().at(0).toByteArray(), QByteArray("PAYLOAD1"));

    link._handleSpartnData("PAYLOAD2");
    QCOMPARE(spartnSpy.count(), 1);
    QCOMPARE(spartnSpy.takeFirst().at(0).toByteArray(), QByteArray("PAYLOAD2"));
}

UT_REGISTER_TEST(GpsTest, TestLabel::Unit)
