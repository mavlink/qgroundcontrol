#include "PX4ULogUtilityTest.h"

#include <cstring>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include <ulog_cpp/messages.hpp>
#include <ulog_cpp/subscription.hpp>
#include <ulog_cpp/writer.hpp>

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

// ============================================================================
// Message Iteration Tests
//
// ULog binary is constructed in-memory using ulog_cpp::Writer — the same
// approach used by the ulog_cpp library's own test suite.  This lets us
// exercise iterateMessages() / MessageHandler against a well-formed stream
// without shipping a binary test fixture file.
//
// Message format used throughout: "test_msg"
//   uint64_t  timestamp  (offset 0, 8 bytes)
//   float     value      (offset 8, 4 bytes)
//   Total payload = 12 bytes
// ============================================================================

namespace {

// Build a payload buffer for "test_msg" from raw values (avoids struct-padding
// issues that would arise from sizeof(struct{uint64_t; float;}) == 16).
std::vector<uint8_t> makeTestMsgPayload(uint64_t timestamp, float value)
{
    std::vector<uint8_t> buf(12);
    memcpy(buf.data(),     &timestamp, 8);
    memcpy(buf.data() + 8, &value,     4);
    return buf;
}

// Build a complete minimal ULog byte stream containing `count` test_msg
// records, each with a sequential timestamp (step 500000 µs) and the given
// base value (incremented by 1.0f per record).
QByteArray buildTestULog(int count, uint64_t baseTimestamp = 500000ULL, float baseValue = 1.0f)
{
    std::vector<uint8_t> buffer;

    // ulog_cpp::Writer is concrete (all DataHandlerInterface methods have
    // default no-op implementations).  We just capture the serialized bytes.
    ulog_cpp::Writer writer([&](const uint8_t *data, int length) {
        buffer.insert(buffer.end(), data, data + length);
    });

    // --- Header section ---
    writer.fileHeader(ulog_cpp::FileHeader{});
    writer.messageFormat(ulog_cpp::MessageFormat{
        "test_msg",
        {ulog_cpp::Field{"uint64_t", "timestamp"}, ulog_cpp::Field{"float", "value"}}
    });
    writer.headerComplete();

    // --- Data section ---
    const uint16_t msgId = 7;  // arbitrary non-zero subscription id
    writer.addLoggedMessage(ulog_cpp::AddLoggedMessage{0, msgId, "test_msg"});

    for (int i = 0; i < count; ++i) {
        const uint64_t ts  = baseTimestamp + static_cast<uint64_t>(i) * 500000ULL;
        const float    val = baseValue + static_cast<float>(i);
        writer.data(ulog_cpp::Data{msgId, makeTestMsgPayload(ts, val)});
    }

    return QByteArray(reinterpret_cast<const char *>(buffer.data()), static_cast<int>(buffer.size()));
}

} // namespace

void PX4ULogUtilityTest::_testIterateMessages()
{
    const QByteArray ulog = buildTestULog(1, 1000000ULL, 3.14f);

    int callCount = 0;
    uint64_t gotTimestamp = 0;
    float    gotValue     = 0.f;

    QString error;
    const bool ok = PX4ULogUtility::iterateMessages(
        ulog.constData(), ulog.size(), "test_msg",
        [&](const ulog_cpp::TypedDataView &sample) {
            ++callCount;
            gotTimestamp = sample.at("timestamp").as<uint64_t>();
            gotValue     = sample.at("value").as<float>();
            return true;
        },
        error);

    QVERIFY2(ok, qPrintable(error));
    QVERIFY(error.isEmpty());
    QCOMPARE(callCount, 1);
    QCOMPARE(gotTimestamp, 1000000ULL);
    QVERIFY2(qAbs(gotValue - 3.14f) < 0.001f,
             qPrintable(QStringLiteral("Expected 3.14, got %1").arg(static_cast<double>(gotValue))));
}

void PX4ULogUtilityTest::_testIterateMessagesMultiple()
{
    constexpr int kCount = 5;
    const QByteArray ulog = buildTestULog(kCount, 100000ULL, 0.0f);

    int callCount = 0;
    QList<uint64_t> timestamps;
    QList<float>    values;

    QString error;
    const bool ok = PX4ULogUtility::iterateMessages(
        ulog.constData(), ulog.size(), "test_msg",
        [&](const ulog_cpp::TypedDataView &sample) {
            ++callCount;
            timestamps.append(sample.at("timestamp").as<uint64_t>());
            values.append(sample.at("value").as<float>());
            return true;
        },
        error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(callCount, kCount);

    for (int i = 0; i < kCount; ++i) {
        const uint64_t expectedTs  = 100000ULL + static_cast<uint64_t>(i) * 500000ULL;
        const float    expectedVal = static_cast<float>(i);
        QCOMPARE(timestamps[i], expectedTs);
        QVERIFY2(qAbs(values[i] - expectedVal) < 0.001f,
                 qPrintable(QStringLiteral("Record %1: expected %2, got %3")
                     .arg(i).arg(static_cast<double>(expectedVal)).arg(static_cast<double>(values[i]))));
    }
}

void PX4ULogUtilityTest::_testIterateMessagesUnknownMessage()
{
    // Valid ULog, but we request a message type that isn't in it.
    const QByteArray ulog = buildTestULog(3);

    int callCount = 0;
    QString error;
    const bool ok = PX4ULogUtility::iterateMessages(
        ulog.constData(), ulog.size(), "nonexistent_msg",
        [&](const ulog_cpp::TypedDataView &) { ++callCount; return true; },
        error);

    QVERIFY2(ok, qPrintable(error));   // parse still succeeds
    QCOMPARE(callCount, 0);            // callback never fires
}

void PX4ULogUtilityTest::_testIterateMessagesFatalError()
{
    // Garbage data that is not a valid ULog stream.
    const QByteArray garbage(64, static_cast<char>(0xFF));

    int callCount = 0;
    QString error;
    const bool ok = PX4ULogUtility::iterateMessages(
        garbage.constData(), garbage.size(), "test_msg",
        [&](const ulog_cpp::TypedDataView &) { ++callCount; return true; },
        error);

    QVERIFY(!ok);
    QCOMPARE(callCount, 0);

    // Empty data — header can never be complete
    callCount = 0;
    QString error2;
    const QByteArray empty;
    QVERIFY(!PX4ULogUtility::iterateMessages(
        empty.constData(), empty.size(), "test_msg",
        [&](const ulog_cpp::TypedDataView &) { ++callCount; return true; }, error2));
    QCOMPARE(callCount, 0);
}

UT_REGISTER_TEST(PX4ULogUtilityTest, TestLabel::Unit, TestLabel::Utilities)
