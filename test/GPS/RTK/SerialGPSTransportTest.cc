#include "SerialGPSTransportTest.h"

#include "SerialGPSTransport.h"

#include <atomic>
#include <cstdint>

void SerialGPSTransportTest::_testReadAbortsWhenStopRequested()
{
    std::atomic_bool stop{true};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);

    uint8_t buffer[16] = {};
    QCOMPARE(transport.read(buffer, static_cast<int>(sizeof(buffer)), 100), -1);
}

void SerialGPSTransportTest::_testWriteAbortsWhenStopRequested()
{
    std::atomic_bool stop{true};
    SerialGPSTransport transport(QStringLiteral("/dev/null"), stop);

    const uint8_t payload[4] = { 1, 2, 3, 4 };
    QCOMPARE(transport.write(payload, static_cast<int>(sizeof(payload))), -1);
}

UT_REGISTER_TEST(SerialGPSTransportTest, TestLabel::Unit)
