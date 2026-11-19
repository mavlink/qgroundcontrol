#include "QGCMathTest.h"

#include <cmath>

#include <QtCore/QtNumeric>

#include "QGCMath.h"
#include "UnitTestMath.h"

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
    QVERIFY(!QGC::fuzzyCompare(inf, -inf));
    QVERIFY(!QGC::fuzzyCompare(-inf, inf));
}

void QGCMathTest::_fuzzyCompareFloat_test()
{
    QVERIFY(QGC::fuzzyCompare(1.0f, 1.0f));
    QVERIFY(QGC::fuzzyCompare(0.0f, 0.0f));

    const float nan = std::numeric_limits<float>::quiet_NaN();
    QVERIFY(QGC::fuzzyCompare(nan, nan));
    QVERIFY(!QGC::fuzzyCompare(nan, 0.0f));
    QVERIFY(!QGC::fuzzyCompare(0.0f, nan));

    const float inf = std::numeric_limits<float>::infinity();
    QVERIFY(QGC::fuzzyCompare(inf, inf));
    QVERIFY(!QGC::fuzzyCompare(inf, -inf));
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

    const double inf = std::numeric_limits<double>::infinity();
    QVERIFY(QGC::fuzzyCompare(inf, inf, 0.1));
    QVERIFY(QGC::fuzzyCompare(-inf, -inf, 0.1));
    QVERIFY(!QGC::fuzzyCompare(inf, -inf, 0.1));
    QVERIFY(!QGC::fuzzyCompare(inf, 1.0, 0.1));
}

void QGCMathTest::_fuzzyCompareFloatCustomTolerance_test()
{
    QVERIFY(QGC::fuzzyCompare(1.0f, 1.05f, 0.1f));
    QVERIFY(!QGC::fuzzyCompare(1.0f, 1.2f, 0.1f));

    const float nan = std::numeric_limits<float>::quiet_NaN();
    QVERIFY(QGC::fuzzyCompare(nan, nan, 0.1f));
    QVERIFY(!QGC::fuzzyCompare(nan, 0.0f, 0.1f));

    const float inf = std::numeric_limits<float>::infinity();
    QVERIFY(QGC::fuzzyCompare(inf, inf, 0.1f));
    QVERIFY(!QGC::fuzzyCompare(inf, -inf, 0.1f));
}

void QGCMathTest::_limitAngleToPMPI_test_data()
{
    QTest::addColumn<double>("angle");
    QTest::addColumn<double>("expected");

    QTest::newRow("0") << 0.0 << 0.0;
    QTest::newRow("pi") << M_PI << M_PI;
    QTest::newRow("-pi") << -M_PI << -M_PI;
    QTest::newRow("2pi") << 2.0 * M_PI << 0.0;
    QTest::newRow("-2pi") << -2.0 * M_PI << 0.0;
    QTest::newRow("-3pi") << -3.0 * M_PI << -M_PI;
    // Angles of 10 turns or more, where the wrap no longer steps one turn at a time
    QTest::newRow("20pi") << 20.0 * M_PI << 0.0;
    QTest::newRow("-20pi") << -20.0 * M_PI << 0.0;
    QTest::newRow("21.5pi") << 21.5 * M_PI << -0.5 * M_PI;
    QTest::newRow("-21.5pi") << -21.5 * M_PI << 0.5 * M_PI;
    QTest::newRow("100pi") << 100.0 * M_PI << 0.0;
    QTest::newRow("-1000.25pi") << -1000.25 * M_PI << -0.25 * M_PI;
    QTest::newRow("below-21pi") << 21.0 * M_PI - 0.001 << M_PI - 0.001;
    QTest::newRow("above-21pi") << 21.0 * M_PI + 0.001 << -M_PI + 0.001;
    QTest::newRow("above-minus-21pi") << -21.0 * M_PI + 0.001 << -M_PI + 0.001;
    QTest::newRow("below-minus-21pi") << -21.0 * M_PI - 0.001 << M_PI - 0.001;
}

void QGCMathTest::_limitAngleToPMPI_test()
{
    QFETCH(double, angle);
    QFETCH(double, expected);
    QCOMPARE_FUZZY(static_cast<double>(QGC::limitAngleToPMPIf(angle)), expected, 1e-5);
    QCOMPARE_FUZZY(QGC::limitAngleToPMPId(angle), expected, 1e-10);
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

QGC_REGISTER_PORTABLE_TEST(QGCMathTest, TestLabel::Unit, TestLabel::Utilities)
