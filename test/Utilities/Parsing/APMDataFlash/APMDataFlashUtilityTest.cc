#include "APMDataFlashUtilityTest.h"

#include <bit>
#include <cmath>
#include <cstring>

#include "APMDataFlashUtility.h"

// ============================================================================
// Format Character Size Tests
// ============================================================================

void APMDataFlashUtilityTest::_testFormatCharSize()
{
    // 1-byte types
    QCOMPARE(APMDataFlashUtility::formatCharSize('b'), 1);
    QCOMPARE(APMDataFlashUtility::formatCharSize('B'), 1);
    QCOMPARE(APMDataFlashUtility::formatCharSize('M'), 1);

    // 2-byte types
    QCOMPARE(APMDataFlashUtility::formatCharSize('h'), 2);
    QCOMPARE(APMDataFlashUtility::formatCharSize('H'), 2);
    QCOMPARE(APMDataFlashUtility::formatCharSize('c'), 2);
    QCOMPARE(APMDataFlashUtility::formatCharSize('C'), 2);
    QCOMPARE(APMDataFlashUtility::formatCharSize('g'), 2);  // half-precision float

    // 4-byte types
    QCOMPARE(APMDataFlashUtility::formatCharSize('i'), 4);
    QCOMPARE(APMDataFlashUtility::formatCharSize('I'), 4);
    QCOMPARE(APMDataFlashUtility::formatCharSize('e'), 4);
    QCOMPARE(APMDataFlashUtility::formatCharSize('E'), 4);
    QCOMPARE(APMDataFlashUtility::formatCharSize('L'), 4);
    QCOMPARE(APMDataFlashUtility::formatCharSize('f'), 4);
    QCOMPARE(APMDataFlashUtility::formatCharSize('n'), 4);

    // 8-byte types
    QCOMPARE(APMDataFlashUtility::formatCharSize('d'), 8);
    QCOMPARE(APMDataFlashUtility::formatCharSize('q'), 8);
    QCOMPARE(APMDataFlashUtility::formatCharSize('Q'), 8);

    // 16-byte types
    QCOMPARE(APMDataFlashUtility::formatCharSize('N'), 16);

    // 64-byte types
    QCOMPARE(APMDataFlashUtility::formatCharSize('Z'), 64);
    QCOMPARE(APMDataFlashUtility::formatCharSize('a'), 64);
}

void APMDataFlashUtilityTest::_testFormatCharSizeUnknown()
{
    QCOMPARE(APMDataFlashUtility::formatCharSize('x'), 0);
    QCOMPARE(APMDataFlashUtility::formatCharSize('?'), 0);
    QCOMPARE(APMDataFlashUtility::formatCharSize('\0'), 0);
}

void APMDataFlashUtilityTest::_testCalculatePayloadSize()
{
    // Empty format
    QCOMPARE(APMDataFlashUtility::calculatePayloadSize(QString()), 0);

    // Single type
    QCOMPARE(APMDataFlashUtility::calculatePayloadSize(QStringLiteral("B")), 1);
    QCOMPARE(APMDataFlashUtility::calculatePayloadSize(QStringLiteral("Q")), 8);

    // Mixed format (typical CAM message: QBILLefffff)
    // Q(8) + B(1) + I(4) + L(4) + L(4) + e(4) + f(4) + f(4) + f(4) + f(4) + f(4) = 45
    QCOMPARE(APMDataFlashUtility::calculatePayloadSize(QStringLiteral("QBILLefffff")), 45);
}

// ============================================================================
// Value Parsing Tests
// ============================================================================

void APMDataFlashUtilityTest::_testParseValue_data()
{
    QTest::addColumn<char>("format");
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<QVariant>("expected");

    QTest::newRow("int8") << 'b' << QByteArray::fromHex("d6") << QVariant(-42);
    QTest::newRow("uint8") << 'B' << QByteArray::fromHex("c8") << QVariant(200);
    QTest::newRow("mode") << 'M' << QByteArray::fromHex("c8") << QVariant(200);
    QTest::newRow("int16") << 'h' << QByteArray::fromHex("2efb") << QVariant(-1234);
    QTest::newRow("uint16") << 'H' << QByteArray::fromHex("50c3") << QVariant(50000);
    QTest::newRow("int32") << 'i' << QByteArray::fromHex("c01dfeff") << QVariant(-123456);
    QTest::newRow("uint32") << 'I' << QByteArray::fromHex("00286bee") << QVariant(4000000000u);
    QTest::newRow("int64") << 'q' << QByteArray::fromHex("35fb048ee0feffff") << QVariant(-1234567890123LL);
    QTest::newRow("uint64") << 'Q' << QByteArray::fromHex("f22fce733a0b0000") << QVariant(12345678901234ULL);
    QTest::newRow("centi-degrees") << 'c' << QByteArray::fromHex("9411") << QVariant(45.0);
    QTest::newRow("unsigned-centi-degrees") << 'C' << QByteArray::fromHex("d204") << QVariant(12.34);
    QTest::newRow("centi-units") << 'e' << QByteArray::fromHex("40e20100") << QVariant(1234.56);
    QTest::newRow("unsigned-centi-units") << 'E' << QByteArray::fromHex("780ae305") << QVariant(987654.32);
    QTest::newRow("latitude") << 'L' << QByteArray::fromHex("500a8016") << QVariant(37.749);
    QTest::newRow("float") << 'f' << QByteArray::fromHex("d00f4940") << QVariant(static_cast<double>(3.14159f));
    QTest::newRow("double") << 'd' << QByteArray::fromHex("6957148b0abf0540") << QVariant(2.718281828459045);
    QTest::newRow("half") << 'g' << QByteArray::fromHex("003c") << QVariant(1.0);
    QTest::newRow("raw-array") << 'a' << QByteArray(64, '\x95') << QVariant(QByteArray(64, '\x95'));
}

void APMDataFlashUtilityTest::_testParseValue()
{
    QFETCH(char, format);
    QFETCH(QByteArray, bytes);
    QFETCH(QVariant, expected);
    const QByteArray unaligned = QByteArray(1, '\xff') + bytes;
    const QVariant value = APMDataFlashUtility::parseValue(unaligned.constData() + 1, bytes.size(), format);
    QCOMPARE(value, expected);
    QCOMPARE(value.metaType(), expected.metaType());
}

void APMDataFlashUtilityTest::_testParseValueInvalid_data()
{
    QTest::addColumn<char>("format");
    QTest::addColumn<int>("size");
    QTest::addColumn<bool>("nullData");
    for (char format : QByteArray("bBMhHcCiIeELfdqQgnNZa")) {
        const QByteArray name = QByteArray("truncated-") + format;
        QTest::newRow(name.constData()) << format << (APMDataFlashUtility::formatCharSize(format) - 1) << false;
    }
    QTest::newRow("negative-size") << 'Q' << -1 << false;
    QTest::newRow("null-data") << 'Q' << 8 << true;
    QTest::newRow("unsupported-format") << '?' << 64 << false;
}

void APMDataFlashUtilityTest::_testParseValueInvalid()
{
    QFETCH(char, format);
    QFETCH(int, size);
    QFETCH(bool, nullData);
    const QByteArray bytes(64, '\0');
    const QString warning = format == '?' ? QStringLiteral("^Unsupported DataFlash format character:")
                                          : QStringLiteral("^Missing or truncated DataFlash value for format:");
    expectLogMessage("Utilities.APMDataFlashUtility", QtWarningMsg, QRegularExpression(warning));
    QVERIFY(!APMDataFlashUtility::parseValue(nullData ? nullptr : bytes.constData(), size, format).isValid());
    verifyExpectedLogMessage();
}

void APMDataFlashUtilityTest::_testParseValueStrings()
{
    // 4-char string (n)
    char n4[4] = {'T', 'E', 'S', 'T'};
    QCOMPARE(APMDataFlashUtility::parseValue(n4, sizeof(n4), 'n').toString(), QStringLiteral("TEST"));

    // 4-char string with null terminator
    char n4null[4] = {'A', 'B', '\0', 'D'};
    QCOMPARE(APMDataFlashUtility::parseValue(n4null, sizeof(n4null), 'n').toString(), QStringLiteral("AB"));

    // 16-char string (N)
    char n16[16] = "HelloWorld123";
    QCOMPARE(APMDataFlashUtility::parseValue(n16, sizeof(n16), 'N').toString(), QStringLiteral("HelloWorld123"));

    // 64-char string (Z)
    char z64[64] = {};
    memset(z64, 0, sizeof(z64));
    strcpy(z64, "This is a longer test string");
    QCOMPARE(APMDataFlashUtility::parseValue(z64, sizeof(z64), 'Z').toString(),
             QStringLiteral("This is a longer test string"));
}

// ============================================================================
// Half-Precision Float Tests
// ============================================================================

void APMDataFlashUtilityTest::_testHalfToFloat_data()
{
    QTest::addColumn<quint16>("bits");
    QTest::addColumn<quint32>("floatBits");
    QTest::newRow("zero") << quint16{0x0000} << quint32{0x00000000};
    QTest::newRow("negative-zero") << quint16{0x8000} << quint32{0x80000000};
    QTest::newRow("smallest-subnormal") << quint16{0x0001} << quint32{0x33800000};
    QTest::newRow("negative-smallest-subnormal") << quint16{0x8001} << quint32{0xb3800000};
    QTest::newRow("largest-subnormal") << quint16{0x03ff} << quint32{0x387fc000};
    QTest::newRow("negative-largest-subnormal") << quint16{0x83ff} << quint32{0xb87fc000};
    QTest::newRow("smallest-normal") << quint16{0x0400} << quint32{0x38800000};
    QTest::newRow("negative-smallest-normal") << quint16{0x8400} << quint32{0xb8800000};
    QTest::newRow("one") << quint16{0x3c00} << quint32{0x3f800000};
    QTest::newRow("negative-one") << quint16{0xbc00} << quint32{0xbf800000};
    QTest::newRow("two") << quint16{0x4000} << quint32{0x40000000};
    QTest::newRow("half") << quint16{0x3800} << quint32{0x3f000000};
    QTest::newRow("largest-finite") << quint16{0x7bff} << quint32{0x477fe000};
    QTest::newRow("negative-largest-finite") << quint16{0xfbff} << quint32{0xc77fe000};
    QTest::newRow("infinity") << quint16{0x7c00} << quint32{0x7f800000};
    QTest::newRow("negative-infinity") << quint16{0xfc00} << quint32{0xff800000};
}

void APMDataFlashUtilityTest::_testHalfToFloat()
{
    QFETCH(quint16, bits);
    QFETCH(quint32, floatBits);
    // Exact bits distinguish subnormals and signed zero without fuzzy floating-point comparisons.
    QCOMPARE(std::bit_cast<quint32>(APMDataFlashUtility::halfToFloat(bits)), floatBits);
    const char wire[] = {'\xff', static_cast<char>(bits & 0xff), static_cast<char>(bits >> 8)};
    const QVariant value = APMDataFlashUtility::parseValue(wire + 1, 2, 'g');
    QVERIFY(value.isValid());
    QCOMPARE(std::bit_cast<quint32>(static_cast<float>(value.toDouble())), floatBits);
}

void APMDataFlashUtilityTest::_testHalfToFloatSpecial()
{
    // NaN (exponent=31, mantissa!=0)
    float nan = APMDataFlashUtility::halfToFloat(0x7C01);
    QVERIFY(std::isnan(nan));
}

// ============================================================================
// Header Validation Tests
// ============================================================================

void APMDataFlashUtilityTest::_testIsValidHeader()
{
    // Valid DataFlash header
    char valid[] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(128)};
    QVERIFY(APMDataFlashUtility::isValidHeader(valid, sizeof(valid)));
}

void APMDataFlashUtilityTest::_testIsValidHeaderInvalid()
{
    // Wrong magic bytes
    char invalid1[] = {static_cast<char>(0xA3), static_cast<char>(0x96), static_cast<char>(128)};
    QVERIFY(!APMDataFlashUtility::isValidHeader(invalid1, sizeof(invalid1)));

    // Too small
    char small[] = {static_cast<char>(0xA3), static_cast<char>(0x95)};
    QVERIFY(!APMDataFlashUtility::isValidHeader(small, 2));

    // Empty
    QVERIFY(!APMDataFlashUtility::isValidHeader(nullptr, 0));
}

void APMDataFlashUtilityTest::_testFindNextHeader()
{
    // Create test data with embedded headers
    char data[20];
    memset(data, 0, sizeof(data));
    // Header at offset 0
    data[0] = static_cast<char>(0xA3);
    data[1] = static_cast<char>(0x95);
    data[2] = 1;  // message type
    // Header at offset 10
    data[10] = static_cast<char>(0xA3);
    data[11] = static_cast<char>(0x95);
    data[12] = 2;  // message type

    QCOMPARE(APMDataFlashUtility::findNextHeader(data, sizeof(data), 0), 0LL);
    QCOMPARE(APMDataFlashUtility::findNextHeader(data, sizeof(data), 1), 10LL);
    QCOMPARE(APMDataFlashUtility::findNextHeader(data, sizeof(data), 11), -1LL);
}

// ============================================================================
// FMT Message Parsing Tests
// ============================================================================

void APMDataFlashUtilityTest::_testParseFmtPayload()
{
    // Create a minimal FMT payload (86 bytes)
    // Type(1) + Length(1) + Name(4) + Format(16) + Columns(64)
    char payload[86];
    memset(payload, 0, sizeof(payload));

    payload[0] = 100;                                        // type
    payload[1] = 25;                                         // length
    memcpy(payload + 2, "CAM\0", 4);                         // name
    memcpy(payload + 6, "QBILLff\0\0\0\0\0\0\0\0\0", 16);    // format
    memcpy(payload + 22, "TimeUS,Img,Lat,Lng,Alt,R,P", 27);  // columns (including null)

    const APMDataFlashUtility::MessageFormat fmt = APMDataFlashUtility::parseFmtPayload(payload);

    QCOMPARE(fmt.type, static_cast<uint8_t>(100));
    QCOMPARE(fmt.length, static_cast<uint8_t>(25));
    QCOMPARE(fmt.name, QStringLiteral("CAM"));
    QCOMPARE(fmt.format, QStringLiteral("QBILLff"));
    QCOMPARE(fmt.columns.size(), 7);
    QCOMPARE(fmt.columns[0], QStringLiteral("TimeUS"));
    QCOMPARE(fmt.columns[1], QStringLiteral("Img"));
    QCOMPARE(fmt.columns[2], QStringLiteral("Lat"));
}

void APMDataFlashUtilityTest::_testParseFmtMessages()
{
    // This requires a real DataFlash log buffer with FMT messages
    // We'll use a minimal synthetic test
    QByteArray data;

    // Add header + FMT message for FMT itself (type 128)
    data.append(static_cast<char>(0xA3));
    data.append(static_cast<char>(0x95));
    data.append(static_cast<char>(128));  // FMT message type

    // FMT payload for FMT message definition
    char fmtPayload[86];
    memset(fmtPayload, 0, sizeof(fmtPayload));
    fmtPayload[0] = static_cast<char>(0x80);  // type for FMT
    fmtPayload[1] = 89;                       // length (3 header + 86 payload)
    memcpy(fmtPayload + 2, "FMT\0", 4);
    memcpy(fmtPayload + 6, "BBnNZ\0\0\0\0\0\0\0\0\0\0\0", 16);
    memcpy(fmtPayload + 22, "Type,Length,Name,Format,Columns", 32);
    data.append(fmtPayload, sizeof(fmtPayload));

    QMap<uint8_t, APMDataFlashUtility::MessageFormat> formats;
    QVERIFY(APMDataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));
    QVERIFY(formats.contains(128));
    QCOMPARE(formats[128].name, QStringLiteral("FMT"));
}

// ============================================================================
// Message Parsing Tests
// ============================================================================

void APMDataFlashUtilityTest::_testParseMessage()
{
    // Create a simple message format
    APMDataFlashUtility::MessageFormat fmt;
    fmt.type = 100;
    fmt.length = 13;  // 3 header + 10 payload (8+1+1)
    fmt.name = QStringLiteral("TEST");
    fmt.format = QStringLiteral("QBb");
    fmt.columns = QStringList() << QStringLiteral("TimeUS") << QStringLiteral("Value1") << QStringLiteral("Value2");

    // Create payload: Q(8 bytes) + B(1 byte) + b(1 byte) = 10 bytes
    const QByteArray payload = QByteArray::fromHex("d2029649000000002af6");
    const QMap<QString, QVariant> fields = APMDataFlashUtility::parseMessage(payload.constData(), payload.size(), fmt);

    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[QStringLiteral("TimeUS")].toULongLong(), 1234567890ULL);
    QCOMPARE(fields[QStringLiteral("Value1")].toUInt(), 42u);
    QCOMPARE(fields[QStringLiteral("Value2")].toInt(), -10);
}

void APMDataFlashUtilityTest::_testParseMessageTruncated_data()
{
    QTest::addColumn<int>("size");
    QTest::newRow("missing-timestamp") << 7;
    QTest::newRow("missing-second-field") << 8;
    QTest::newRow("missing-last-field") << 9;
}

void APMDataFlashUtilityTest::_testParseMessageTruncated()
{
    QFETCH(int, size);
    APMDataFlashUtility::MessageFormat fmt;
    fmt.format = QStringLiteral("QBb");
    fmt.columns = {QStringLiteral("TimeUS"), QStringLiteral("Value1"), QStringLiteral("Value2")};
    const QByteArray payload = QByteArray::fromHex("d2029649000000002af6");
    expectLogMessage("Utilities.APMDataFlashUtility", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Missing or truncated DataFlash value for format:")));
    QVERIFY(APMDataFlashUtility::parseMessage(payload.constData(), size, fmt).isEmpty());
    verifyExpectedLogMessage();
}

void APMDataFlashUtilityTest::_testParseMessageUnsupportedFormat_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<int>("columnCount");
    QTest::newRow("first-field") << QStringLiteral("?I") << 2;
    QTest::newRow("middle-field") << QStringLiteral("I?I") << 3;
    QTest::newRow("last-field") << QStringLiteral("II?") << 3;
    QTest::newRow("unnamed-tail") << QStringLiteral("I?") << 1;
}

void APMDataFlashUtilityTest::_testParseMessageUnsupportedFormat()
{
    QFETCH(QString, format);
    QFETCH(int, columnCount);
    APMDataFlashUtility::MessageFormat fmt;
    fmt.format = format;
    for (int i = 0; i < columnCount; ++i) {
        fmt.columns.append(QString::number(i));
    }
    const QByteArray payload = QByteArray::fromHex("010000002a000000");
    expectLogMessage("Utilities.APMDataFlashUtility", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Unsupported DataFlash format character:")));
    QVERIFY(APMDataFlashUtility::parseMessage(payload.constData(), payload.size(), fmt).isEmpty());
    verifyExpectedLogMessage();
}

// ============================================================================
// Message Iteration Tests
// ============================================================================

void APMDataFlashUtilityTest::_testIterateMessages()
{
    // Create a minimal DataFlash buffer with:
    // 1. FMT message defining TEST (type 200)
    // 2. Two TEST messages

    QByteArray data;

    // First: FMT message (header + 86-byte payload)
    data.append(static_cast<char>(0xA3));
    data.append(static_cast<char>(0x95));
    data.append(static_cast<char>(128));  // FMT type

    char fmtPayload[86];
    memset(fmtPayload, 0, sizeof(fmtPayload));
    fmtPayload[0] = static_cast<char>(200);  // type for TEST
    fmtPayload[1] = 12;                      // length (3 header + 9 payload)
    memcpy(fmtPayload + 2, "TEST", 4);
    memcpy(fmtPayload + 6, "QBx", 3);        // Note: 'x' is unknown, should be skipped
    memcpy(fmtPayload + 22, "TimeUS,Val", 10);
    data.append(fmtPayload, sizeof(fmtPayload));

    // Parse formats first
    QMap<uint8_t, APMDataFlashUtility::MessageFormat> formats;
    QVERIFY(APMDataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));

    // Add two TEST messages
    for (int i = 0; i < 2; ++i) {
        data.append(static_cast<char>(0xA3));
        data.append(static_cast<char>(0x95));
        data.append(static_cast<char>(200));  // TEST type

        char msgPayload[9];
        uint64_t ts = 1000000 + i * 100000;
        memcpy(msgPayload, &ts, 8);
        msgPayload[8] = static_cast<char>(i + 1);
        data.append(msgPayload, sizeof(msgPayload));
    }

    // Re-parse formats to include the TEST format
    formats.clear();
    QVERIFY(APMDataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));

    // Iterate messages
    int messageCount = 0;
    QList<uint8_t> msgTypes;

    APMDataFlashUtility::iterateMessages(data.constData(), data.size(), formats,
                                      [&](uint8_t msgType, const char*, int, const APMDataFlashUtility::MessageFormat&) {
                                          ++messageCount;
                                          msgTypes.append(msgType);
                                          return true;
                                      });

    // Should have found 2 TEST messages (FMT messages are only used to build format table)
    QCOMPARE(messageCount, 2);
    QCOMPARE(msgTypes[0], static_cast<uint8_t>(200));  // TEST
    QCOMPARE(msgTypes[1], static_cast<uint8_t>(200));  // TEST
}

void APMDataFlashUtilityTest::_testIterateMessagesProgress()
{
    // Build a buffer with enough messages to cross the 1000-message threshold at least once.
    QByteArray data;

    // FMT message for a small record type (type 200, 12 bytes total = 9 byte payload)
    data.append(static_cast<char>(0xA3));
    data.append(static_cast<char>(0x95));
    data.append(static_cast<char>(128));
    char fmtPayload[86];
    memset(fmtPayload, 0, sizeof(fmtPayload));
    fmtPayload[0] = static_cast<char>(200);
    fmtPayload[1] = 12;
    memcpy(fmtPayload + 2, "TEST", 4);
    memcpy(fmtPayload + 6, "Qb", 2);
    memcpy(fmtPayload + 22, "TimeUS,Val", 10);
    data.append(fmtPayload, sizeof(fmtPayload));

    QMap<uint8_t, APMDataFlashUtility::MessageFormat> formats;
    QVERIFY(APMDataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));

    // Append 2000 TEST messages so the progress callback fires at least once (every 1000 messages)
    for (int i = 0; i < 2000; ++i) {
        data.append(static_cast<char>(0xA3));
        data.append(static_cast<char>(0x95));
        data.append(static_cast<char>(200));
        char msg[9];
        uint64_t ts = static_cast<uint64_t>(i) * 1000;
        memcpy(msg, &ts, 8);
        msg[8] = static_cast<char>(i & 0xFF);
        data.append(msg, 9);
    }

    formats.clear();
    QVERIFY(APMDataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));

    QList<float> progressValues;
    int messageCount = 0;

    APMDataFlashUtility::iterateMessages(
        data.constData(), data.size(), formats,
        [&](uint8_t, const char *, int, const APMDataFlashUtility::MessageFormat &) {
            ++messageCount;
            return true;
        },
        [&](float v) {
            progressValues.append(v);
        });

    QCOMPARE(messageCount, 2000);

    // Progress callback must have fired at least once (at 1000-message mark)
    QVERIFY(!progressValues.isEmpty());

    // All values must be in (0, 1]
    for (float v : progressValues) {
        QVERIFY(v > 0.f);
        QVERIFY(v <= 1.f);
    }

    // Values must be monotonically non-decreasing
    for (int i = 1; i < progressValues.size(); ++i) {
        QVERIFY(progressValues[i] >= progressValues[i - 1]);
    }
}

UT_REGISTER_TEST(APMDataFlashUtilityTest, TestLabel::Unit, TestLabel::Utilities)
