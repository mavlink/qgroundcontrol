#include "QGCFormatTest.h"

#include <QtCore/QLocale>

#include "QGCFormat.h"
#include "UnitTest.h"

void QGCFormatTest::_numberToStringZero_test()
{
    const QLocale locale;
    QCOMPARE(QGC::numberToString(0), locale.toString(static_cast<quint64>(0)));
}

void QGCFormatTest::_numberToStringLarge_test()
{
    const QLocale locale;
    const quint64 value = 1234567ULL;
    QCOMPARE(QGC::numberToString(value), locale.toString(value));
}

void QGCFormatTest::_bigSizeToStringBytes_test()
{
    const QLocale locale;
    QCOMPARE(QGC::bigSizeToString(0), locale.toString(static_cast<quint64>(0)) + QStringLiteral("B"));
    QCOMPARE(QGC::bigSizeToString(512), locale.toString(static_cast<quint64>(512)) + QStringLiteral("B"));
    QCOMPARE(QGC::bigSizeToString(1023), locale.toString(static_cast<quint64>(1023)) + QStringLiteral("B"));
}

void QGCFormatTest::_bigSizeToStringKB_test()
{
    const QLocale locale;
    QCOMPARE(QGC::bigSizeToString(1024), locale.toString(1.0, 'f', 1) + QStringLiteral("KB"));
    QCOMPARE(QGC::bigSizeToString(1536), locale.toString(1.5, 'f', 1) + QStringLiteral("KB"));
}

void QGCFormatTest::_bigSizeToStringMB_test()
{
    const QLocale locale;
    const quint64 oneMB = 1024ULL * 1024ULL;
    QCOMPARE(QGC::bigSizeToString(oneMB), locale.toString(1.0, 'f', 1) + QStringLiteral("MB"));
}

void QGCFormatTest::_bigSizeMBToStringMB_test()
{
    const QLocale locale;
    QCOMPARE(QGC::bigSizeMBToString(0), locale.toString(0.0, 'f', 0) + QStringLiteral(" MB"));
    QCOMPARE(QGC::bigSizeMBToString(512), locale.toString(512.0, 'f', 0) + QStringLiteral(" MB"));
    QCOMPARE(QGC::bigSizeMBToString(1023), locale.toString(1023.0, 'f', 0) + QStringLiteral(" MB"));
}

void QGCFormatTest::_bigSizeMBToStringGB_test()
{
    const QLocale locale;
    QCOMPARE(QGC::bigSizeMBToString(1024), locale.toString(1.0, 'f', 1) + QStringLiteral(" GB"));
    QCOMPARE(QGC::bigSizeMBToString(1536), locale.toString(1.5, 'f', 1) + QStringLiteral(" GB"));
}

UT_REGISTER_TEST(QGCFormatTest, TestLabel::Unit, TestLabel::Utilities)
