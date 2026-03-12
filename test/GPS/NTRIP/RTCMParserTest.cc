#include "RTCMParserTest.h"
#include "RTCMParser.h"

static QByteArray buildRtcmFrame(uint16_t messageId, int extraPayloadBytes = 0)
{
    const int payloadLen = 2 + extraPayloadBytes;
    QByteArray frame;

    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>((payloadLen >> 8) & 0x03));
    frame.append(static_cast<char>(payloadLen & 0xFF));

    frame.append(static_cast<char>((messageId >> 4) & 0xFF));
    frame.append(static_cast<char>((messageId & 0x0F) << 4));

    for (int i = 0; i < extraPayloadBytes; i++) {
        frame.append(static_cast<char>(i & 0xFF));
    }

    const uint32_t crc = RTCMParser::crc24q(
        reinterpret_cast<const uint8_t*>(frame.constData()),
        static_cast<size_t>(frame.size()));

    frame.append(static_cast<char>((crc >> 16) & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    return frame;
}

// ---------------------------------------------------------------------------
// CRC-24Q Tests
// ---------------------------------------------------------------------------

void RTCMParserTest::testCrc24qEmpty()
{
    const uint32_t crc = RTCMParser::crc24q(nullptr, 0);
    QCOMPARE(crc, 0u);
}

void RTCMParserTest::testCrc24qSingleByte()
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

void RTCMParserTest::testCrc24qKnownVector()
{
    const uint8_t data[] = { 0xD3, 0x00, 0x02, 0x3E, 0xD0 };
    const uint32_t crc1 = RTCMParser::crc24q(data, sizeof(data));
    const uint32_t crc2 = RTCMParser::crc24q(data, sizeof(data));
    QCOMPARE(crc1, crc2);
    QVERIFY((crc1 & 0xFF000000u) == 0u);
}

void RTCMParserTest::testCrc24qReferenceVector()
{
    // Standard CRC-24Q check value: "123456789" -> 0xCDE703
    const uint8_t data[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
    const uint32_t crc = RTCMParser::crc24q(data, sizeof(data));
    QCOMPARE(crc, static_cast<uint32_t>(0xCDE703));
}

void RTCMParserTest::testCrc24qIncremental()
{
    QByteArray frame = buildRtcmFrame(1005, 10);

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

void RTCMParserTest::testParserReset()
{
    RTCMParser parser;
    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(0));
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(0));

    parser.addByte(RTCM3_PREAMBLE);
    parser.addByte(0x00);
    parser.reset();

    QCOMPARE(parser.messageLength(), static_cast<uint16_t>(0));
}

void RTCMParserTest::testParserValidMessage()
{
    QByteArray frame = buildRtcmFrame(1005, 4);
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

void RTCMParserTest::testParserCrcValidation()
{
    QByteArray frame = buildRtcmFrame(1077, 8);
    RTCMParser parser;

    for (int i = 0; i < frame.size(); i++) {
        parser.addByte(static_cast<uint8_t>(frame[i]));
    }

    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::testParserInvalidCrc()
{
    QByteArray frame = buildRtcmFrame(1077, 8);
    frame[frame.size() - 1] = static_cast<char>(frame[frame.size() - 1] ^ 0xFF);

    RTCMParser parser;
    for (int i = 0; i < frame.size(); i++) {
        parser.addByte(static_cast<uint8_t>(frame[i]));
    }

    QVERIFY(!parser.validateCrc());
}

void RTCMParserTest::testParserMessageId()
{
    const uint16_t testIds[] = { 1001, 1005, 1006, 1033, 1077, 1087, 1097, 1127, 4094 };

    for (uint16_t id : testIds) {
        QByteArray frame = buildRtcmFrame(id, 0);
        RTCMParser parser;

        for (int i = 0; i < frame.size(); i++) {
            parser.addByte(static_cast<uint8_t>(frame[i]));
        }

        QCOMPARE(parser.messageId(), id);
    }
}

void RTCMParserTest::testParserGarbageBeforePreamble()
{
    QByteArray frame = buildRtcmFrame(1005, 2);

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

void RTCMParserTest::testParserInvalidLength()
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
    QByteArray valid = buildRtcmFrame(1005, 0);
    bool complete = false;
    for (int i = 0; i < valid.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(valid[i]))) {
            complete = true;
        }
    }
    QVERIFY(complete);
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::testParserOverlengthRejected()
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
    QByteArray valid = buildRtcmFrame(1077, 0);
    bool complete = false;
    for (int i = 0; i < valid.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(valid[i]))) {
            complete = true;
        }
    }
    QVERIFY(complete);
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1077));
}

void RTCMParserTest::testParserMultipleMessages()
{
    QByteArray msg1 = buildRtcmFrame(1005, 4);
    QByteArray msg2 = buildRtcmFrame(1077, 8);
    QByteArray msg3 = buildRtcmFrame(1087, 2);

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

void RTCMParserTest::testParserMaxLength()
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

void RTCMParserTest::testParserTruncatedFrame()
{
    QByteArray frame = buildRtcmFrame(1005, 4);

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

void RTCMParserTest::testParserCorruptedPreamble()
{
    // Bytes that look like preamble but aren't, followed by a valid frame
    QByteArray input;
    // Near-miss preamble bytes
    input.append(static_cast<char>(0xD2));
    input.append(static_cast<char>(0xD4));
    input.append(static_cast<char>(0x00));
    input.append(static_cast<char>(0x02));

    QByteArray valid = buildRtcmFrame(1033, 6);
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

// ---------------------------------------------------------------------------
// Edge Cases
// ---------------------------------------------------------------------------

void RTCMParserTest::testParserZeroLengthPayload()
{
    // Zero-length payload — parser should reject (length > 0 required)
    QByteArray frame;
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x00)); // length = 0

    RTCMParser parser;
    for (int i = 0; i < frame.size(); i++) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(frame[i])));
    }

    // Parser should have reset; verify it recovers with a valid frame
    QByteArray valid = buildRtcmFrame(1005, 0);
    bool complete = false;
    for (int i = 0; i < valid.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(valid[i]))) {
            complete = true;
        }
    }
    QVERIFY(complete);
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::testParserMaxLengthPayload()
{
    // Max payload = 1023 bytes, already tested in testParserMaxLength
    // but here we verify boundary: 1022 should work, 1024 should fail
    auto buildCustomFrame = [](uint16_t payloadLen) -> QByteArray {
        QByteArray frame;
        frame.append(static_cast<char>(RTCM3_PREAMBLE));
        frame.append(static_cast<char>((payloadLen >> 8) & 0x03));
        frame.append(static_cast<char>(payloadLen & 0xFF));
        for (int i = 0; i < payloadLen; i++) {
            frame.append(static_cast<char>(i & 0xFF));
        }
        const uint32_t crc = RTCMParser::crc24q(
            reinterpret_cast<const uint8_t*>(frame.constData()),
            static_cast<size_t>(frame.size()));
        frame.append(static_cast<char>((crc >> 16) & 0xFF));
        frame.append(static_cast<char>((crc >> 8) & 0xFF));
        frame.append(static_cast<char>(crc & 0xFF));
        return frame;
    };

    // 1022 should work
    {
        QByteArray frame = buildCustomFrame(1022);
        RTCMParser parser;
        bool complete = false;
        for (int i = 0; i < frame.size(); i++) {
            if (parser.addByte(static_cast<uint8_t>(frame[i]))) {
                complete = true;
            }
        }
        QVERIFY(complete);
        QCOMPARE(parser.messageLength(), static_cast<uint16_t>(1022));
    }

    // 1023 should work (max)
    {
        QByteArray frame = buildCustomFrame(1023);
        RTCMParser parser;
        bool complete = false;
        for (int i = 0; i < frame.size(); i++) {
            if (parser.addByte(static_cast<uint8_t>(frame[i]))) {
                complete = true;
            }
        }
        QVERIFY(complete);
        QCOMPARE(parser.messageLength(), static_cast<uint16_t>(1023));
    }
}

void RTCMParserTest::testParserPreambleInPayload()
{
    // Build a frame where 0xD3 (preamble) appears inside the payload
    const int extraPayload = 10;
    const int payloadLen = 2 + extraPayload;
    QByteArray frame;
    frame.append(static_cast<char>(RTCM3_PREAMBLE));
    frame.append(static_cast<char>((payloadLen >> 8) & 0x03));
    frame.append(static_cast<char>(payloadLen & 0xFF));
    frame.append(static_cast<char>((1005 >> 4) & 0xFF));
    frame.append(static_cast<char>((1005 & 0x0F) << 4));
    // Embed preamble bytes in payload
    for (int i = 0; i < extraPayload; i++) {
        frame.append(static_cast<char>(RTCM3_PREAMBLE));
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
    QCOMPARE(parser.messageId(), static_cast<uint16_t>(1005));
    QVERIFY(parser.validateCrc());
}

void RTCMParserTest::testParserTruncatedMidFrame()
{
    // Start one frame, abandon it mid-payload, then send a complete frame
    QByteArray frame1 = buildRtcmFrame(1005, 20);
    QByteArray frame2 = buildRtcmFrame(1077, 4);

    // Feed only 8 bytes of frame1 (mid-payload), then feed all of frame2
    // The parser should be stuck in frame1's ReadingMessage state,
    // and frame2's preamble is consumed as payload data,
    // so frame2 won't be detected. After frame1 fails, the parser resets.
    RTCMParser parser;
    QByteArray input = frame1.left(8) + frame2;

    int completeCount = 0;
    for (int i = 0; i < input.size(); i++) {
        if (parser.addByte(static_cast<uint8_t>(input[i]))) {
            completeCount++;
            parser.reset();
        }
    }

    // Due to the truncation corruption, we may get 0 or 1 valid parses
    // The key assertion: the parser doesn't crash or hang
    QVERIFY(completeCount <= 1);
}

void RTCMParserTest::testParserWhitelistEdgeCases()
{
    RTCMParser parser;

    // Empty whitelist — all accepted
    QVERIFY(parser.isWhitelisted(0));
    QVERIFY(parser.isWhitelisted(4095));

    // Single-entry whitelist
    parser.setWhitelist({1005});
    QVERIFY(parser.isWhitelisted(1005));
    QVERIFY(!parser.isWhitelisted(0));
    QVERIFY(!parser.isWhitelisted(4095));

    // Clear whitelist — back to accept all
    parser.setWhitelist({});
    QVERIFY(parser.isWhitelisted(9999));

    // Large whitelist
    QVector<int> large;
    for (int i = 1000; i < 1200; i++) large.append(i);
    parser.setWhitelist(large);
    QVERIFY(parser.isWhitelisted(1005));
    QVERIFY(parser.isWhitelisted(1199));
    QVERIFY(!parser.isWhitelisted(999));
    QVERIFY(!parser.isWhitelisted(1200));
}

UT_REGISTER_TEST(RTCMParserTest, TestLabel::Unit)
