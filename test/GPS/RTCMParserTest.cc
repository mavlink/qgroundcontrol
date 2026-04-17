#include "RTCMParserTest.h"
#include "RTCMParser.h"
#include "GpsTestHelpers.h"

// ---------------------------------------------------------------------------
// CRC-24Q Tests
// ---------------------------------------------------------------------------

void RTCMParserTest::_testCrc24qEmpty()
{
    const uint32_t crc = RTCMParser::crc24q(nullptr, 0);
    QCOMPARE(crc, 0u);
}

void RTCMParserTest::_testCrc24qSingleByte()
{
    // CRC-24Q of 0x00 with initial value 0 is 0 (no bits set to trigger polynomial)
    const uint8_t zero = 0x00;
    QCOMPARE(RTCMParser::crc24q(&zero, 1), 0u);

    // CRC-24Q of a non-zero byte should produce a non-zero 24-bit result
    const uint8_t nonzero = 0x01;
    const uint32_t crc = RTCMParser::crc24q(&nonzero, 1);
    QVERIFY(crc != 0u);
    QCOMPARE(crc & 0xFF000000u, 0u);
}

void RTCMParserTest::_testCrc24qKnownVector()
{
    const uint8_t data[] = { 0xD3, 0x00, 0x02, 0x3E, 0xD0 };
    const uint32_t crc1 = RTCMParser::crc24q(data, sizeof(data));
    const uint32_t crc2 = RTCMParser::crc24q(data, sizeof(data));
    QCOMPARE(crc1, crc2);
    QVERIFY((crc1 & 0xFF000000u) == 0u);
}

void RTCMParserTest::_testCrc24qReferenceVector()
{
    // Standard CRC-24Q check value: "123456789" -> 0xCDE703
    const uint8_t data[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
    const uint32_t crc = RTCMParser::crc24q(data, sizeof(data));
    QCOMPARE(crc, static_cast<uint32_t>(0xCDE703));
}

void RTCMParserTest::_testCrc24qIncremental()
{
    QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 10);

    // header(3) + payload(12) + crc(3) = 18
    QCOMPARE(frame.size(), 18);

    const uint32_t frameCrc = (static_cast<uint8_t>(frame[15]) << 16) |
                              (static_cast<uint8_t>(frame[16]) << 8) |
                               static_cast<uint8_t>(frame[17]);

    const uint32_t computed = RTCMParser::crc24q(
        reinterpret_cast<const uint8_t*>(frame.constData()), 15);

    QCOMPARE(computed, frameCrc);
}

// ---------------------------------------------------------------------------
// RTCMParser State Machine Tests
// ---------------------------------------------------------------------------

void RTCMParserTest::_testParserReset()
{
    RTCMParser parser;
    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(0));
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(0));

    parser.addByte(RTCM3_PREAMBLE);
    parser.addByte(0x00);
    parser.reset();

    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(0));
}

void RTCMParserTest::_testParserValidMessage()
{
    QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);
    RTCMParser parser;

    bool complete = false;
    for (int i = 0; i < frame.size(); i++) {
        complete = parser.addByte(static_cast<uint8_t>(frame[i]));
        if (i < frame.size() - 1) {
            QVERIFY2(!complete, qPrintable(QString("Parser reported complete at byte %1").arg(i)));
        }
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(6));
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1005));
}

void RTCMParserTest::_testParserCrcValidation()
{
    QByteArray frame = GpsTestHelpers::buildRtcmFrame(1077, 8);
    RTCMParser parser;

    for (int i = 0; i < frame.size(); i++) {
        parser.addByte(static_cast<uint8_t>(frame[i]));
    }

    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::_testParserInvalidCrc()
{
    QByteArray frame = GpsTestHelpers::buildRtcmFrame(1077, 8);
    frame[frame.size() - 1] = static_cast<char>(frame[frame.size() - 1] ^ 0xFF);

    RTCMParser parser;
    for (int i = 0; i < frame.size(); i++) {
        parser.addByte(static_cast<uint8_t>(frame[i]));
    }

    QVERIFY(!parser.validateCrc());
}

void RTCMParserTest::_testParserMessageId()
{
    const uint16_t testIds[] = { 1001, 1005, 1006, 1033, 1077, 1087, 1097, 1127, 4094 };

    for (uint16_t id : testIds) {
        QByteArray frame = GpsTestHelpers::buildRtcmFrame(id, 0);
        RTCMParser parser;

        for (int i = 0; i < frame.size(); i++) {
            parser.addByte(static_cast<uint8_t>(frame[i]));
        }

        QCOMPARE(parser.messageId(), id);
    }
}

void RTCMParserTest::_testParserGarbageBeforePreamble()
{
    QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 2);

    QByteArray input;
    input.append('\x00');
    input.append('\xFF');
    input.append('\xAA');
    input.append('\x55');
    input.append(frame);

    RTCMParser parser;
    bool complete = false;
    for (int i = 0; i < input.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(input[i]))) {
            complete = true;
            break;
        }
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1005));
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::_testParserInvalidLength()
{
    // Length = 0 is invalid; parser should reset
    QByteArray frame;
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append('\x00');
    frame.append('\x00');

    RTCMParser parser;
    for (int i = 0; i < frame.size(); i++) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(frame[i])));
    }

    // Parser should recover and parse a valid message
    QByteArray valid = GpsTestHelpers::buildRtcmFrame(1005, 0);
    bool complete = false;
    for (int i = 0; i < valid.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(valid[i]))) {
            complete = true;
        }
    }
    QVERIFY(complete);
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::_testParserOverlengthRejected()
{
    // Length > 1023 (10-bit max) — set reserved bits in length field
    QByteArray frame;
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>(0x04)); // bit 2 set = 1024 (exceeds 1023)
    frame.append(static_cast<char>(0x00));

    RTCMParser parser;
    for (int i = 0; i < frame.size(); i++) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(frame[i])));
    }

    // Parser should have reset; verify recovery
    QByteArray valid = GpsTestHelpers::buildRtcmFrame(1077, 0);
    bool complete = false;
    for (int i = 0; i < valid.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(valid[i]))) {
            complete = true;
        }
    }
    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1077));
}

void RTCMParserTest::_testParserMultipleMessages()
{
    QByteArray msg1 = GpsTestHelpers::buildRtcmFrame(1005, 4);
    QByteArray msg2 = GpsTestHelpers::buildRtcmFrame(1077, 8);
    QByteArray msg3 = GpsTestHelpers::buildRtcmFrame(1087, 2);

    QByteArray stream = msg1 + msg2 + msg3;

    RTCMParser parser;
    int messageCount = 0;
    QVector<uint16_t> ids;

    for (int i = 0; i < stream.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(stream[i]))) {
            QVERIFY(parser.validateCrc());
            ids.append(parser.messageId());
            messageCount++;
            parser.reset();
        }
    }

    QCOMPARE(messageCount, 3);
    QCOMPARE(ids[0], static_cast<uint16_t>(1005));
    QCOMPARE(ids[1], static_cast<uint16_t>(1077));
    QCOMPARE(ids[2], static_cast<uint16_t>(1087));
}

void RTCMParserTest::_testParserMaxLength()
{
    QByteArray frame;
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>(0x03)); // 1023 upper
    frame.append(static_cast<char>(0xFF)); // 1023 lower

    // 1023 bytes of payload
    frame.append(static_cast<char>(0x3E)); // message ID 1005 upper
    frame.append(static_cast<char>(0xD0)); // message ID 1005 lower
    for (int i = 0; i < 1021; i++) {
        frame.append(static_cast<char>(i & 0xFF));
    }

    const uint32_t crc = RTCMParser::crc24q(
        reinterpret_cast<const uint8_t*>(frame.constData()),
        static_cast<size_t>(frame.size()));
    frame.append(static_cast<char>((crc >> 16) & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    RTCMParser parser;
    bool complete = false;
    for (int i = 0; i < frame.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(frame[i]))) {
            complete = true;
        }
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(1023));
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1005));
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::_testParserTruncatedFrame()
{
    QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005, 4);

    // Feed only half the frame; parser should never report complete
    RTCMParser parser;
    const int halfSize = frame.size() / 2;
    for (int i = 0; i < halfSize; i++) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(frame[i])));
    }

    // After reset, a full valid frame should still parse
    parser.reset();
    bool complete = false;
    for (int i = 0; i < frame.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(frame[i]))) {
            complete = true;
        }
    }
    QVERIFY(complete);
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::_testParserCorruptedPreamble()
{
    // Bytes that look like preamble but aren't, followed by a valid frame
    QByteArray input;
    // Near-miss preamble bytes
    input.append(static_cast<char>(0xD2));
    input.append(static_cast<char>(0xD4));
    input.append(static_cast<char>(0x00));
    input.append(static_cast<char>(0x02));

    QByteArray valid = GpsTestHelpers::buildRtcmFrame(1033, 6);
    input.append(valid);

    RTCMParser parser;
    bool complete = false;
    for (int i = 0; i < input.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(input[i]))) {
            complete = true;
            break;
        }
    }

    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1033));
    QVERIFY(parser.validateCrc());
}

UT_REGISTER_TEST(RTCMParserTest, TestLabel::Unit)
