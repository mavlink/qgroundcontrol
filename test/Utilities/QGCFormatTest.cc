#include "QGCFormatTest.h"

#include <QtCore/QLocale>

#include "QGCFormat.h"
#include "UnitTest.h"

namespace {

/// Pin formatting against the C locale so unit suffixes stay stable in CI.
class ScopedCLocale
{
public:
    ScopedCLocale()
        : _previous(QLocale())
    {
        QLocale::setDefault(QLocale::c());
    }

    ~ScopedCLocale() { QLocale::setDefault(_previous); }

private:
    QLocale _previous;
};

} // namespace

void QGCFormatTest::_numberToStringBasic()
{
    const ScopedCLocale localeGuard;

    QCOMPARE(QGC::numberToString(0), QStringLiteral("0"));
    QCOMPARE(QGC::numberToString(42), QStringLiteral("42"));
    QCOMPARE(QGC::numberToString(1000), QStringLiteral("1000"));
}

void QGCFormatTest::_bigSizeToStringBoundaries()
{
    const ScopedCLocale localeGuard;

    // Bytes (< 1 KiB)
    QCOMPARE(QGC::bigSizeToString(0), QStringLiteral("0B"));
    QCOMPARE(QGC::bigSizeToString(512), QStringLiteral("512B"));
    QCOMPARE(QGC::bigSizeToString(1023), QStringLiteral("1023B"));

    // KiB band
    QCOMPARE(QGC::bigSizeToString(1024), QStringLiteral("1.0KB"));
    QCOMPARE(QGC::bigSizeToString(1536), QStringLiteral("1.5KB"));

    // MiB / GiB / TiB lower edges
    QCOMPARE(QGC::bigSizeToString(1024ULL * 1024ULL), QStringLiteral("1.0MB"));
    QCOMPARE(QGC::bigSizeToString(1024ULL * 1024ULL * 1024ULL), QStringLiteral("1.0GB"));
    QCOMPARE(QGC::bigSizeToString(1024ULL * 1024ULL * 1024ULL * 1024ULL), QStringLiteral("1.0TB"));
}

void QGCFormatTest::_bigSizeMBToStringBoundaries()
{
    const ScopedCLocale localeGuard;

    // Input is already in mebibytes. Below 1024 MB → "N MB" (no fractional digit).
    QCOMPARE(QGC::bigSizeMBToString(0), QStringLiteral("0 MB"));
    QCOMPARE(QGC::bigSizeMBToString(1), QStringLiteral("1 MB"));
    QCOMPARE(QGC::bigSizeMBToString(1023), QStringLiteral("1023 MB"));

    // 1024 MB = 1.0 GB
    QCOMPARE(QGC::bigSizeMBToString(1024), QStringLiteral("1.0 GB"));
    QCOMPARE(QGC::bigSizeMBToString(1536), QStringLiteral("1.5 GB"));

    // 1024*1024 MB = 1.00 TB
    QCOMPARE(QGC::bigSizeMBToString(1024ULL * 1024ULL), QStringLiteral("1.00 TB"));
}

UT_REGISTER_TEST(QGCFormatTest, TestLabel::Unit, TestLabel::Utilities)
