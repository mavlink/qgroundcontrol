#include "RTCMParserTest.h"

#include "RTCMParser.h"
#include "RTCMTestFixtures.h"

void RTCMParserTest::_frameAccess_data()
{
    QTest::addColumn<int>("messageId");
    QTest::addColumn<int>("extraPayload");
    QTest::newRow("minimum-id") << 0 << 0;
    QTest::newRow("base-position") << 1005 << 4;
    QTest::newRow("observations") << 1077 << 8;
    QTest::newRow("maximum-id") << 4095 << 0;
    QTest::newRow("near-maximum-length") << 1230 << 1020;
    QTest::newRow("maximum-length") << 1230 << 1021;
}

void RTCMParserTest::_frameAccess()
{
    QFETCH(int, messageId);
    QFETCH(int, extraPayload);
    const auto frame = GpsTestHelpers::buildRtcmFrame(static_cast<uint16_t>(messageId), extraPayload);
    RTCMParser parser;
    for (qsizetype index = 0; index < frame.size(); ++index) {
        QCOMPARE(parser.addByte(static_cast<uint8_t>(frame[index])), index == frame.size() - 1);
    }
    QVERIFY(parser.validateCrc());
    QCOMPARE(parser.messageId(), messageId);
    QCOMPARE(parser.messageLength(), extraPayload + 2);
    QCOMPARE(parser.currentFrame(), frame);
    QCOMPARE(
        QByteArray(reinterpret_cast<const char*>(parser.message()), RTCMParser::kHeaderSize + parser.messageLength()),
        frame.chopped(RTCMParser::kCrcSize));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(parser.crcBytes()), RTCMParser::kCrcSize),
             frame.last(RTCMParser::kCrcSize));

    const auto savedFrame = parser.currentFrame();
    parser.reset();
    const auto replacement = GpsTestHelpers::buildRtcmFrame(1006, extraPayload);
    for (const char byte : replacement) {
        parser.addByte(static_cast<uint8_t>(byte));
    }
    QCOMPARE(parser.currentFrame(), replacement);
    QCOMPARE(savedFrame, frame);
}

void RTCMParserTest::_partialAndReset()
{
    const auto frame = QByteArray::fromHex("d300023ed0a4e000");
    RTCMParser parser;
    parser.setWhitelist({1005});
    QVERIFY(parser.currentFrame().isEmpty());
    QVERIFY(!parser.validateCrc());
    for (const char byte : frame.first(frame.size() - 1)) {
        QVERIFY(!parser.addByte(static_cast<uint8_t>(byte)));
        QVERIFY(parser.currentFrame().isEmpty());
        QVERIFY(!parser.validateCrc());
    }
    QVERIFY(parser.addByte(static_cast<uint8_t>(frame.back())));
    QVERIFY(parser.validateCrc());
    parser.reset();
    QVERIFY(parser.currentFrame().isEmpty());
    QVERIFY(!parser.validateCrc());
    QCOMPARE(parser.messageLength(), 0);
    QCOMPARE(parser.messageId(), 0);
    QVERIFY(parser.isWhitelisted(1005));
    QVERIFY(!parser.isWhitelisted(1077));

    QVERIFY(!parser.addByte(RTCMParser::kPreamble));
    parser.reset();
    for (const char byte : frame) {
        parser.addByte(static_cast<uint8_t>(byte));
    }
    QCOMPARE(parser.currentFrame(), frame);
}

void RTCMParserTest::_whitelist()
{
    RTCMParser parser;
    QVERIFY(parser.isWhitelisted(0));
    QVERIFY(parser.isWhitelisted(4095));
    parser.setWhitelist({1005, 1005, 1077});
    QVERIFY(parser.isWhitelisted(1005));
    QVERIFY(parser.isWhitelisted(1077));
    QVERIFY(!parser.isWhitelisted(1087));
    parser.setWhitelist({});
    QVERIFY(parser.isWhitelisted(1087));
}

void RTCMParserTest::_crcCompatibility()
{
    QCOMPARE(RTCMParser::crc24q(nullptr, 0), 0U);
    const auto bytes = QByteArrayLiteral("123456789");
    QCOMPARE(RTCMParser::crc24q(reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())),
             0xcde703U);
}

#ifdef QGC_GPS_STANDALONE_TEST
QTEST_GUILESS_MAIN(RTCMParserTest)
#else
UT_REGISTER_TEST(RTCMParserTest, TestLabel::Unit)
#endif
