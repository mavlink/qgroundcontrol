#include "PX4ULogUtilityTest.h"


#include <cstring>

#include "PX4ULogUtility.h"

// ============================================================================
// Header Validation Tests
// ============================================================================

void PX4ULogUtilityTest::_testIsValidHeader()
{
    // Valid ULog header: "ULog" magic bytes
    char valid[16] = {'U', 'L', 'o', 'g', 0x01, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    QVERIFY(PX4ULogUtility::isValidHeader(valid, sizeof(valid)));
}

void PX4ULogUtilityTest::_testIsValidHeaderInvalid()
{
    // Wrong magic bytes
    char invalid1[8] = {'u', 'l', 'o', 'g', 0x01, 0x12, 0x00, 0x00};
    QVERIFY(!PX4ULogUtility::isValidHeader(invalid1, sizeof(invalid1)));

    // Completely wrong data
    char invalid2[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    QVERIFY(!PX4ULogUtility::isValidHeader(invalid2, sizeof(invalid2)));

    // DataFlash header (not ULog)
    char dataflash[3] = {static_cast<char>(0xA3), static_cast<char>(0x95), static_cast<char>(0x80)};
    QVERIFY(!PX4ULogUtility::isValidHeader(dataflash, sizeof(dataflash)));

    // Too small
    char small[3] = {'U', 'L', 'o'};
    QVERIFY(!PX4ULogUtility::isValidHeader(small, sizeof(small)));

    // Empty
    QVERIFY(!PX4ULogUtility::isValidHeader(nullptr, 0));
}

void PX4ULogUtilityTest::_testIsValidHeaderQByteArray()
{
    // Test QByteArray data with pointer/size API
    QByteArray valid("ULog", 4);
    valid.append('\x01');      // version
    valid.append(11, '\x00');  // rest of header
    QVERIFY(PX4ULogUtility::isValidHeader(valid.constData(), valid.size()));

    QByteArray invalid("NotULog", 7);
    QVERIFY(!PX4ULogUtility::isValidHeader(invalid.constData(), invalid.size()));

    QByteArray empty;
    QVERIFY(!PX4ULogUtility::isValidHeader(empty.constData(), empty.size()));
}

// ============================================================================
// Version Detection Tests
// ============================================================================

void PX4ULogUtilityTest::_testGetVersion()
{
    // Version 1 ULog
    char v1[8] = {'U', 'L', 'o', 'g', 0x01, 0x12, 0x00, 0x00};
    QCOMPARE(PX4ULogUtility::getVersion(v1, sizeof(v1)), 1);

    // Hypothetical version 2
    char v2[8] = {'U', 'L', 'o', 'g', 0x02, 0x12, 0x00, 0x00};
    QCOMPARE(PX4ULogUtility::getVersion(v2, sizeof(v2)), 2);
}

void PX4ULogUtilityTest::_testGetVersionInvalid()
{
    // Invalid magic
    char invalid[8] = {'n', 'o', 't', ' ', 'u', 'l', 'o', 'g'};
    QCOMPARE(PX4ULogUtility::getVersion(invalid, sizeof(invalid)), -1);

    // Too short to contain version
    char short_data[4] = {'U', 'L', 'o', 'g'};
    QCOMPARE(PX4ULogUtility::getVersion(short_data, sizeof(short_data)), -1);

    // Empty
    QCOMPARE(PX4ULogUtility::getVersion(nullptr, 0), -1);
}

// ============================================================================
// Timestamp Tests
// ============================================================================

void PX4ULogUtilityTest::_testGetHeaderTimestamp()
{
    // Create a valid ULog header with timestamp
    char header[16];
    memset(header, 0, sizeof(header));

    // Magic bytes
    header[0] = 'U';
    header[1] = 'L';
    header[2] = 'o';
    header[3] = 'g';

    // Version and flags
    header[4] = 0x01;  // version
    header[5] = 0x12;  // compat flags
    header[6] = 0x00;  // incompat flags
    header[7] = 0x00;

    // Timestamp at offset 8 (8 bytes, little-endian)
    uint64_t timestamp = 1234567890123456ULL;
    memcpy(header + 8, &timestamp, sizeof(timestamp));

    QCOMPARE(PX4ULogUtility::getHeaderTimestamp(header, sizeof(header)), timestamp);

    // Invalid header should return 0
    char invalid[16] = {0};
    QCOMPARE(PX4ULogUtility::getHeaderTimestamp(invalid, sizeof(invalid)), 0ULL);

    // Too small
    QCOMPARE(PX4ULogUtility::getHeaderTimestamp(header, 8), 0ULL);
}

UT_REGISTER_TEST(PX4ULogUtilityTest, TestLabel::Unit, TestLabel::Utilities)
