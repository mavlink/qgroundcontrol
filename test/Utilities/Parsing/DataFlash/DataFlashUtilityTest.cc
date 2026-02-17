#include "DataFlashUtilityTest.h"
#include "DataFlashUtility.h"

#include <QtTest/QTest>

#include <cstring>
#include <cmath>

// ============================================================================
// Format Character Size Tests
// ============================================================================

void DataFlashUtilityTest::_testFormatCharSize()
{
    // 1-byte types
    QCOMPARE(DataFlashUtility::formatCharSize('b'), 1);
    QCOMPARE(DataFlashUtility::formatCharSize('B'), 1);
    QCOMPARE(DataFlashUtility::formatCharSize('M'), 1);

    // 2-byte types
    QCOMPARE(DataFlashUtility::formatCharSize('h'), 2);
    QCOMPARE(DataFlashUtility::formatCharSize('H'), 2);
    QCOMPARE(DataFlashUtility::formatCharSize('c'), 2);
    QCOMPARE(DataFlashUtility::formatCharSize('C'), 2);
    QCOMPARE(DataFlashUtility::formatCharSize('g'), 2);  // half-precision float

    // 4-byte types
    QCOMPARE(DataFlashUtility::formatCharSize('i'), 4);
    QCOMPARE(DataFlashUtility::formatCharSize('I'), 4);
    QCOMPARE(DataFlashUtility::formatCharSize('e'), 4);
    QCOMPARE(DataFlashUtility::formatCharSize('E'), 4);
    QCOMPARE(DataFlashUtility::formatCharSize('L'), 4);
    QCOMPARE(DataFlashUtility::formatCharSize('f'), 4);
    QCOMPARE(DataFlashUtility::formatCharSize('n'), 4);

    // 8-byte types
    QCOMPARE(DataFlashUtility::formatCharSize('d'), 8);
    QCOMPARE(DataFlashUtility::formatCharSize('q'), 8);
    QCOMPARE(DataFlashUtility::formatCharSize('Q'), 8);

    // 16-byte types
    QCOMPARE(DataFlashUtility::formatCharSize('N'), 16);

    // 64-byte types
    QCOMPARE(DataFlashUtility::formatCharSize('Z'), 64);
    QCOMPARE(DataFlashUtility::formatCharSize('a'), 64);
}

void DataFlashUtilityTest::_testFormatCharSizeUnknown()
{
    QCOMPARE(DataFlashUtility::formatCharSize('x'), 0);
    QCOMPARE(DataFlashUtility::formatCharSize('?'), 0);
    QCOMPARE(DataFlashUtility::formatCharSize('\0'), 0);
}

void DataFlashUtilityTest::_testCalculatePayloadSize()
{
    // Empty format
    QCOMPARE(DataFlashUtility::calculatePayloadSize(QString()), 0);

    // Single type
    QCOMPARE(DataFlashUtility::calculatePayloadSize(QStringLiteral("B")), 1);
    QCOMPARE(DataFlashUtility::calculatePayloadSize(QStringLiteral("Q")), 8);

    // Mixed format (typical CAM message: QBILLefffff)
    // Q(8) + B(1) + I(4) + L(4) + L(4) + e(4) + f(4) + f(4) + f(4) + f(4) + f(4) = 45
    QCOMPARE(DataFlashUtility::calculatePayloadSize(QStringLiteral("QBILLefffff")), 45);
}

// ============================================================================
// Value Parsing Tests
// ============================================================================

void DataFlashUtilityTest::_testParseValueIntegers()
{
    // int8
    char i8 = -42;
    QCOMPARE(DataFlashUtility::parseValue(&i8, 'b').toInt(), -42);

    // uint8
    unsigned char u8 = 200;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&u8), 'B').toUInt(), 200u);

    // int16
    int16_t i16 = -1234;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&i16), 'h').toInt(), -1234);

    // uint16
    uint16_t u16 = 50000;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&u16), 'H').toUInt(), 50000u);

    // int32
    int32_t i32 = -123456;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&i32), 'i').toInt(), -123456);

    // uint32
    uint32_t u32 = 4000000000u;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&u32), 'I').toUInt(), 4000000000u);

    // int64
    int64_t i64 = -1234567890123LL;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&i64), 'q').toLongLong(), -1234567890123LL);

    // uint64
    uint64_t u64 = 12345678901234ULL;
    QCOMPARE(DataFlashUtility::parseValue(reinterpret_cast<char*>(&u64), 'Q').toULongLong(), 12345678901234ULL);
}

void DataFlashUtilityTest::_testParseValueScaled()
{
    // Centi-degrees (c) - signed, divide by 100
    int16_t cdeg = 4500;  // 45.00 degrees
    QVERIFY(qAbs(DataFlashUtility::parseValue(reinterpret_cast<char*>(&cdeg), 'c').toDouble() - 45.0) < 0.001);

    // Centi-units (C) - unsigned, divide by 100
    uint16_t cunit = 1234;  // 12.34
    QVERIFY(qAbs(DataFlashUtility::parseValue(reinterpret_cast<char*>(&cunit), 'C').toDouble() - 12.34) < 0.001);

    // Centi-units (e) - signed int32, divide by 100
    int32_t e32 = 123456;  // 1234.56
    QVERIFY(qAbs(DataFlashUtility::parseValue(reinterpret_cast<char*>(&e32), 'e').toDouble() - 1234.56) < 0.001);

    // Centi-units (E) - unsigned int32, divide by 100
    uint32_t E32 = 98765432;  // 987654.32
    QVERIFY(qAbs(DataFlashUtility::parseValue(reinterpret_cast<char*>(&E32), 'E').toDouble() - 987654.32) < 0.01);

    // Latitude/Longitude (L) - divide by 1e7
    int32_t latlon = 377490000;  // 37.749 degrees
    QVERIFY(qAbs(DataFlashUtility::parseValue(reinterpret_cast<char*>(&latlon), 'L').toDouble() - 37.749) < 0.0001);
}

void DataFlashUtilityTest::_testParseValueFloats()
{
    // Single-precision float
    float f32 = 3.14159f;
    double result = DataFlashUtility::parseValue(reinterpret_cast<char*>(&f32), 'f').toDouble();
    QVERIFY2(qAbs(result - 3.14159) < 0.0001,
             qPrintable(QString("Expected ~3.14159, got %1").arg(result)));

    // Double-precision float
    double f64 = 2.718281828459045;
    result = DataFlashUtility::parseValue(reinterpret_cast<char*>(&f64), 'd').toDouble();
    QVERIFY2(qAbs(result - 2.718281828459045) < 0.0000001,
             qPrintable(QString("Expected ~2.718281828459045, got %1").arg(result)));
}

void DataFlashUtilityTest::_testParseValueStrings()
{
    // 4-char string (n)
    char n4[4] = {'T', 'E', 'S', 'T'};
    QCOMPARE(DataFlashUtility::parseValue(n4, 'n').toString(), QStringLiteral("TEST"));

    // 4-char string with null terminator
    char n4null[4] = {'A', 'B', '\0', 'D'};
    QCOMPARE(DataFlashUtility::parseValue(n4null, 'n').toString(), QStringLiteral("AB"));

    // 16-char string (N)
    char n16[16] = "HelloWorld123";
    QCOMPARE(DataFlashUtility::parseValue(n16, 'N').toString(), QStringLiteral("HelloWorld123"));

    // 64-char string (Z)
    char z64[64] = {};
    memset(z64, 0, sizeof(z64));
    strcpy(z64, "This is a longer test string");
    QCOMPARE(DataFlashUtility::parseValue(z64, 'Z').toString(), QStringLiteral("This is a longer test string"));
}

// ============================================================================
// Half-Precision Float Tests
// ============================================================================

void DataFlashUtilityTest::_testHalfToFloat()
{
    // Test common values
    // 1.0 in half-precision: sign=0, exp=15 (biased), mantissa=0 -> 0x3C00
    QVERIFY(qAbs(DataFlashUtility::halfToFloat(0x3C00) - 1.0f) < 0.001f);

    // 2.0 in half-precision: 0x4000
    QVERIFY(qAbs(DataFlashUtility::halfToFloat(0x4000) - 2.0f) < 0.001f);

    // -1.0 in half-precision: 0xBC00
    QVERIFY(qAbs(DataFlashUtility::halfToFloat(0xBC00) - (-1.0f)) < 0.001f);

    // 0.5 in half-precision: 0x3800
    QVERIFY(qAbs(DataFlashUtility::halfToFloat(0x3800) - 0.5f) < 0.001f);

    // Test via parseValue
    uint16_t half = 0x3C00;  // 1.0
    double result = DataFlashUtility::parseValue(reinterpret_cast<char*>(&half), 'g').toDouble();
    QVERIFY2(qAbs(result - 1.0) < 0.001,
             qPrintable(QString("Expected 1.0, got %1").arg(result)));
}

void DataFlashUtilityTest::_testHalfToFloatSpecial()
{
    // Zero
    QCOMPARE(DataFlashUtility::halfToFloat(0x0000), 0.0f);

    // Negative zero
    QCOMPARE(DataFlashUtility::halfToFloat(0x8000), -0.0f);

    // Infinity (exponent=31, mantissa=0)
    float inf = DataFlashUtility::halfToFloat(0x7C00);
    QVERIFY(std::isinf(inf) && inf > 0);

    // Negative infinity
    float ninf = DataFlashUtility::halfToFloat(0xFC00);
    QVERIFY(std::isinf(ninf) && ninf < 0);

    // NaN (exponent=31, mantissa!=0)
    float nan = DataFlashUtility::halfToFloat(0x7C01);
    QVERIFY(std::isnan(nan));
}

// ============================================================================
// Header Validation Tests
// ============================================================================

void DataFlashUtilityTest::_testIsValidHeader()
{
    // Valid DataFlash header
    char valid[] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(128)};
    QVERIFY(DataFlashUtility::isValidHeader(valid, sizeof(valid)));
}

void DataFlashUtilityTest::_testIsValidHeaderInvalid()
{
    // Wrong magic bytes
    char invalid1[] = {static_cast<char>(0xA3), static_cast<char>(0x96), static_cast<char>(128)};
    QVERIFY(!DataFlashUtility::isValidHeader(invalid1, sizeof(invalid1)));

    // Too small
    char small[] = {static_cast<char>(0xA3), static_cast<char>(0x95)};
    QVERIFY(!DataFlashUtility::isValidHeader(small, 2));

    // Empty
    QVERIFY(!DataFlashUtility::isValidHeader(nullptr, 0));
}

void DataFlashUtilityTest::_testFindNextHeader()
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

    QCOMPARE(DataFlashUtility::findNextHeader(data, sizeof(data), 0), 0LL);
    QCOMPARE(DataFlashUtility::findNextHeader(data, sizeof(data), 1), 10LL);
    QCOMPARE(DataFlashUtility::findNextHeader(data, sizeof(data), 11), -1LL);
}

// ============================================================================
// FMT Message Parsing Tests
// ============================================================================

void DataFlashUtilityTest::_testParseFmtPayload()
{
    // Create a minimal FMT payload (86 bytes)
    // Type(1) + Length(1) + Name(4) + Format(16) + Columns(64)
    char payload[86];
    memset(payload, 0, sizeof(payload));

    payload[0] = 100;                    // type
    payload[1] = 25;                     // length
    memcpy(payload + 2, "CAM\0", 4);     // name
    memcpy(payload + 6, "QBILLff\0\0\0\0\0\0\0\0\0", 16);  // format
    memcpy(payload + 22, "TimeUS,Img,Lat,Lng,Alt,R,P", 27); // columns (including null)

    const DataFlashUtility::MessageFormat fmt = DataFlashUtility::parseFmtPayload(payload);

    QCOMPARE(fmt.type, static_cast<uint8_t>(100));
    QCOMPARE(fmt.length, static_cast<uint8_t>(25));
    QCOMPARE(fmt.name, QStringLiteral("CAM"));
    QCOMPARE(fmt.format, QStringLiteral("QBILLff"));
    QCOMPARE(fmt.columns.size(), 7);
    QCOMPARE(fmt.columns[0], QStringLiteral("TimeUS"));
    QCOMPARE(fmt.columns[1], QStringLiteral("Img"));
    QCOMPARE(fmt.columns[2], QStringLiteral("Lat"));
}

void DataFlashUtilityTest::_testParseFmtMessages()
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
    fmtPayload[1] = 89;   // length (3 header + 86 payload)
    memcpy(fmtPayload + 2, "FMT\0", 4);
    memcpy(fmtPayload + 6, "BBnNZ\0\0\0\0\0\0\0\0\0\0\0", 16);
    memcpy(fmtPayload + 22, "Type,Length,Name,Format,Columns", 32);
    data.append(fmtPayload, sizeof(fmtPayload));

    QMap<uint8_t, DataFlashUtility::MessageFormat> formats;
    QVERIFY(DataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));
    QVERIFY(formats.contains(128));
    QCOMPARE(formats[128].name, QStringLiteral("FMT"));
}

// ============================================================================
// Message Parsing Tests
// ============================================================================

void DataFlashUtilityTest::_testParseMessage()
{
    // Create a simple message format
    DataFlashUtility::MessageFormat fmt;
    fmt.type = 100;
    fmt.length = 13;  // 3 header + 10 payload (8+1+1)
    fmt.name = QStringLiteral("TEST");
    fmt.format = QStringLiteral("QBb");
    fmt.columns = QStringList() << QStringLiteral("TimeUS")
                                << QStringLiteral("Value1")
                                << QStringLiteral("Value2");

    // Create payload: Q(8 bytes) + B(1 byte) + b(1 byte) = 10 bytes
    char payload[10];
    uint64_t timestamp = 1234567890;
    memcpy(payload, &timestamp, 8);
    payload[8] = static_cast<char>(42);   // uint8
    payload[9] = static_cast<char>(-10);  // int8

    const QMap<QString, QVariant> fields = DataFlashUtility::parseMessage(payload, fmt);

    QCOMPARE(fields.size(), 3);
    QCOMPARE(fields[QStringLiteral("TimeUS")].toULongLong(), 1234567890ULL);
    QCOMPARE(fields[QStringLiteral("Value1")].toUInt(), 42u);
    QCOMPARE(fields[QStringLiteral("Value2")].toInt(), -10);
}

// ============================================================================
// Message Iteration Tests
// ============================================================================

void DataFlashUtilityTest::_testIterateMessages()
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
    fmtPayload[1] = 12;  // length (3 header + 9 payload)
    memcpy(fmtPayload + 2, "TEST", 4);
    memcpy(fmtPayload + 6, "QBx", 3);  // Note: 'x' is unknown, should be skipped
    memcpy(fmtPayload + 22, "TimeUS,Val", 10);
    data.append(fmtPayload, sizeof(fmtPayload));

    // Parse formats first
    QMap<uint8_t, DataFlashUtility::MessageFormat> formats;
    QVERIFY(DataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));

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
    QVERIFY(DataFlashUtility::parseFmtMessages(data.constData(), data.size(), formats));

    // Iterate messages
    int messageCount = 0;
    QList<uint8_t> msgTypes;

    DataFlashUtility::iterateMessages(data.constData(), data.size(), formats,
        [&](uint8_t msgType, const char *, int, const DataFlashUtility::MessageFormat &) {
            ++messageCount;
            msgTypes.append(msgType);
            return true;
        });

    // Should have found 2 TEST messages (FMT messages are only used to build format table)
    QCOMPARE(messageCount, 2);
    QCOMPARE(msgTypes[0], static_cast<uint8_t>(200));  // TEST
    QCOMPARE(msgTypes[1], static_cast<uint8_t>(200));  // TEST
}

UT_REGISTER_TEST(DataFlashUtilityTest, TestLabel::Unit, TestLabel::Utilities)
