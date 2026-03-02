#include "QGCMathTest.h"

#include <QtCore/QtNumeric>

#include <cmath>

#include "QGC.h"

void QGCMathTest::_fuzzyCompareDoubleEqual_test()
{
    QVERIFY(QGC::fuzzyCompare(1.0, 1.0));
    QVERIFY(QGC::fuzzyCompare(0.0, 0.0));
    QVERIFY(QGC::fuzzyCompare(-1.0, -1.0));
    QVERIFY(QGC::fuzzyCompare(1e10, 1e10));
}

void QGCMathTest::_fuzzyCompareDoubleZero_test()
{
    QVERIFY(QGC::fuzzyCompare(0.0, 0.0));
    QVERIFY(QGC::fuzzyCompare(-0.0, 0.0));
    // Values very close to zero but not exactly zero should fail default fuzzyCompare
    // because qFuzzyCompare requires values to be relatively close (not absolutely close)
    QVERIFY(!QGC::fuzzyCompare(0.0, 1e-5));
}

void QGCMathTest::_fuzzyCompareDoubleNaN_test()
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    QVERIFY(QGC::fuzzyCompare(nan, nan));
    QVERIFY(!QGC::fuzzyCompare(nan, 0.0));
    QVERIFY(!QGC::fuzzyCompare(0.0, nan));
    QVERIFY(!QGC::fuzzyCompare(nan, 1.0));
}

void QGCMathTest::_fuzzyCompareDoubleInf_test()
{
    const double inf = std::numeric_limits<double>::infinity();
    QVERIFY(QGC::fuzzyCompare(inf, inf));
    QVERIFY(QGC::fuzzyCompare(-inf, -inf));
    QVERIFY(!QGC::fuzzyCompare(inf, 1.0));
}

void QGCMathTest::_fuzzyCompareFloat_test()
{
    QVERIFY(QGC::fuzzyCompare(1.0f, 1.0f));
    QVERIFY(QGC::fuzzyCompare(0.0f, 0.0f));

    const float nan = std::numeric_limits<float>::quiet_NaN();
    QVERIFY(QGC::fuzzyCompare(nan, nan));
    QVERIFY(!QGC::fuzzyCompare(nan, 0.0f));
    QVERIFY(!QGC::fuzzyCompare(0.0f, nan));
}

void QGCMathTest::_fuzzyCompareCustomTolerance_test()
{
    QVERIFY(QGC::fuzzyCompare(1.0, 1.05, 0.1));
    QVERIFY(!QGC::fuzzyCompare(1.0, 1.2, 0.1));
    QVERIFY(QGC::fuzzyCompare(0.0, 0.0, 0.001));
    QVERIFY(QGC::fuzzyCompare(0.0, 0.0005, 0.001));

    const double nan = std::numeric_limits<double>::quiet_NaN();
    QVERIFY(QGC::fuzzyCompare(nan, nan, 0.1));
    QVERIFY(!QGC::fuzzyCompare(nan, 0.0, 0.1));
}

void QGCMathTest::_fuzzyCompareFloatCustomTolerance_test()
{
    QVERIFY(QGC::fuzzyCompare(1.0f, 1.05f, 0.1f));
    QVERIFY(!QGC::fuzzyCompare(1.0f, 1.2f, 0.1f));

    const float nan = std::numeric_limits<float>::quiet_NaN();
    QVERIFY(QGC::fuzzyCompare(nan, nan, 0.1f));
    QVERIFY(!QGC::fuzzyCompare(nan, 0.0f, 0.1f));
}

void QGCMathTest::_limitAngleToPMPIfZero_test()
{
    QCOMPARE_FUZZY(static_cast<double>(QGC::limitAngleToPMPIf(0.0)), 0.0, 1e-5);
}

void QGCMathTest::_limitAngleToPMPIfBoundary_test()
{
    QCOMPARE_FUZZY(static_cast<double>(QGC::limitAngleToPMPIf(M_PI)), M_PI, 1e-4);
    QCOMPARE_FUZZY(static_cast<double>(QGC::limitAngleToPMPIf(-M_PI)), -M_PI, 1e-4);
}

void QGCMathTest::_limitAngleToPMPIfWrap_test()
{
    const float result = QGC::limitAngleToPMPIf(2.0 * M_PI);
    QVERIFY(result >= -static_cast<float>(M_PI) - 1e-4f);
    QVERIFY(result <= static_cast<float>(M_PI) + 1e-4f);
    QCOMPARE_FUZZY(static_cast<double>(result), 0.0, 1e-4);

    const float resultNeg = QGC::limitAngleToPMPIf(-2.0 * M_PI);
    QVERIFY(resultNeg >= -static_cast<float>(M_PI) - 1e-4f);
    QVERIFY(resultNeg <= static_cast<float>(M_PI) + 1e-4f);
}

void QGCMathTest::_limitAngleToPMPIfLargeAngle_test()
{
    // For very large angles (> 20*pi), the function uses fmod approximation
    const float result = QGC::limitAngleToPMPIf(100.0 * M_PI);
    QVERIFY(result >= -static_cast<float>(M_PI) - 1e-4f);
    QVERIFY(result <= static_cast<float>(M_PI) + 1e-4f);
}

void QGCMathTest::_limitAngleToPMPIdZero_test()
{
    QCOMPARE_FUZZY(QGC::limitAngleToPMPId(0.0), 0.0, 1e-10);
}

void QGCMathTest::_limitAngleToPMPIdBoundary_test()
{
    QCOMPARE_FUZZY(QGC::limitAngleToPMPId(M_PI), M_PI, 1e-10);
    QCOMPARE_FUZZY(QGC::limitAngleToPMPId(-M_PI), -M_PI, 1e-10);
}

void QGCMathTest::_limitAngleToPMPIdWrap_test()
{
    const double result = QGC::limitAngleToPMPId(2.0 * M_PI);
    QVERIFY(result >= -M_PI - 1e-10);
    QVERIFY(result <= M_PI + 1e-10);
    QCOMPARE_FUZZY(result, 0.0, 1e-10);

    const double resultNeg = QGC::limitAngleToPMPId(-3.0 * M_PI);
    QVERIFY(resultNeg >= -M_PI - 1e-10);
    QVERIFY(resultNeg <= M_PI + 1e-10);
}

void QGCMathTest::_limitAngleToPMPIdLargeAngle_test()
{
    const double result = QGC::limitAngleToPMPId(100.0 * M_PI);
    QVERIFY(result >= -M_PI - 1e-10);
    QVERIFY(result <= M_PI + 1e-10);
}

void QGCMathTest::_crc32Empty_test()
{
    const quint32 crc = QGC::crc32(nullptr, 0, 0);
    QCOMPARE(crc, 0u);
}

void QGCMathTest::_crc32KnownVector_test()
{
    // Standard CRC-32 of "123456789": init=0xFFFFFFFF, final XOR=0xFFFFFFFF -> 0xCBF43926
    const QByteArray data = QByteArrayLiteral("123456789");
    const quint32 crc = QGC::crc32(reinterpret_cast<const quint8 *>(data.constData()),
                                    static_cast<unsigned>(data.size()), 0xFFFFFFFF) ^ 0xFFFFFFFF;
    QCOMPARE(crc, 0xCBF43926u);
}

void QGCMathTest::_crc32Incremental_test()
{
    // Incremental CRC must produce the same result as single-pass
    const QByteArray data = QByteArrayLiteral("123456789");
    quint32 crc = QGC::crc32(reinterpret_cast<const quint8 *>(data.constData()), 4, 0xFFFFFFFF);
    crc = QGC::crc32(reinterpret_cast<const quint8 *>(data.constData() + 4),
                      static_cast<unsigned>(data.size() - 4), crc);
    crc ^= 0xFFFFFFFF;
    QCOMPARE(crc, 0xCBF43926u);
}

UT_REGISTER_TEST(QGCMathTest, TestLabel::Unit, TestLabel::Utilities)
