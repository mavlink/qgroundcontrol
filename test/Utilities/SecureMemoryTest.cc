#include "SecureMemoryTest.h"

#include <QtCore/QByteArray>
#include <array>
#include <cstring>

#include "SecureMemory.h"
#include "UnitTest.h"

void SecureMemoryTest::_testSecureZeroRawMemory()
{
    constexpr size_t kSize = 64;
    unsigned char buf[kSize];
    memset(buf, 0xAB, kSize);

    QGC::secureZero(buf, kSize);

    for (size_t i = 0; i < kSize; ++i) {
        QCOMPARE(buf[i], static_cast<unsigned char>(0));
    }
}

void SecureMemoryTest::_testSecureZeroByteArray()
{
    QByteArray data(32, '\xFF');
    QCOMPARE(data.size(), 32);

    QGC::secureZero(data);

    QVERIFY(data.isEmpty());
}

void SecureMemoryTest::_testSecureZeroEmptyByteArray()
{
    QByteArray data;
    QGC::secureZero(data);
    QVERIFY(data.isEmpty());
}

void SecureMemoryTest::_testSecureZeroZeroSize()
{
    QGC::secureZero(nullptr, 0);
}

void SecureMemoryTest::_testSecureZeroStdArray()
{
    std::array<uint8_t, 32> key{};
    key.fill(0xCD);

    QGC::secureZero(key);

    for (const auto byte : key) {
        QCOMPARE(byte, static_cast<uint8_t>(0));
    }
}

UT_REGISTER_TEST(SecureMemoryTest, TestLabel::Unit, TestLabel::Utilities)
