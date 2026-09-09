#include "SerialGPSTransportTest.h"

#include "SerialGPSTransport.h"

#include <atomic>
#include <cstdint>

void SerialGPSTransportTest::_testReadAbortsWhenStopRequested()
{
    std::atomic_bool stop{false};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);
    QVERIFY(!transport.isCancelled());
    stop = true;

    uint8_t buffer[16] = {};
    QCOMPARE(transport.read(buffer, static_cast<int>(sizeof(buffer)), 100), -1);
    QVERIFY(transport.isCancelled());
}

void SerialGPSTransportTest::_testWriteAbortsWhenStopRequested()
{
    std::atomic_bool stop{false};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);
    QVERIFY(!transport.isCancelled());
    stop = true;

    const uint8_t payload[4] = { 1, 2, 3, 4 };
    QCOMPARE(transport.write(payload, static_cast<int>(sizeof(payload))), -1);
}

UT_REGISTER_TEST(SerialGPSTransportTest, TestLabel::Unit)
