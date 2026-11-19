#include "QGCFormatTest.h"

#include <limits>

#include <QtCore/QLocale>

#include "QGCFormat.h"

namespace {
constexpr quint64 QGC_FORMAT_TEST_MAX = std::numeric_limits<quint64>::max();

/// The functions under test format with the default locale, so each test sets its own.
class QGCFormatTestLocale
{
public:
    explicit QGCFormatTestLocale(const QString& name) { QLocale::setDefault(QLocale(name)); }

    ~QGCFormatTestLocale() { QLocale::setDefault(_previous); }

private:
    Q_DISABLE_COPY_MOVE(QGCFormatTestLocale)

    const QLocale _previous;  // default-constructed: the default locale before this object changed it
};

void addQGCFormatTestColumns()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<quint64>("value");
    QTest::addColumn<QString>("expected");
}
}  // namespace

void QGCFormatTest::_numberToString_data()
{
    addQGCFormatTestColumns();
    QTest::newRow("zero") << QStringLiteral("en_US") << quint64(0) << QStringLiteral("0");
    QTest::newRow("below-first-group") << QStringLiteral("en_US") << quint64(999) << QStringLiteral("999");
    QTest::newRow("first-group") << QStringLiteral("en_US") << quint64(1000) << QStringLiteral("1,000");
    QTest::newRow("largest-value") << QStringLiteral("en_US") << QGC_FORMAT_TEST_MAX
                                   << QStringLiteral("18,446,744,073,709,551,615");
    QTest::newRow("german-group-separator")
        << QStringLiteral("de_DE") << quint64(1234567) << QStringLiteral("1.234.567");
    QTest::newRow("c-locale-has-no-groups") << QStringLiteral("C") << quint64(1234567) << QStringLiteral("1234567");
}

void QGCFormatTest::_numberToString()
{
    QFETCH(QString, locale);
    QFETCH(quint64, value);
    QFETCH(QString, expected);

    const QGCFormatTestLocale defaultLocale(locale);
    QCOMPARE(QGC::numberToString(value), expected);
}

void QGCFormatTest::_bigSizeToString_data()
{
    addQGCFormatTestColumns();
    QTest::newRow("zero") << QStringLiteral("en_US") << quint64(0) << QStringLiteral("0B");
    QTest::newRow("largest-byte-count") << QStringLiteral("en_US") << quint64(1023) << QStringLiteral("1,023B");
    QTest::newRow("smallest-KB") << QStringLiteral("en_US") << quint64(1024) << QStringLiteral("1.0KB");
    QTest::newRow("rounds-down") << QStringLiteral("en_US") << quint64(1075) << QStringLiteral("1.0KB");
    QTest::newRow("rounds-up") << QStringLiteral("en_US") << quint64(1076) << QStringLiteral("1.1KB");
    // A size that rounds to 1,024.0 of a unit is shown as 1.0 of the next one
    QTest::newRow("largest-KB") << QStringLiteral("en_US") << quint64(1048524) << QStringLiteral("1,023.9KB");
    QTest::newRow("rounds-to-MB") << QStringLiteral("en_US") << quint64(1048525) << QStringLiteral("1.0MB");
    QTest::newRow("below-MB") << QStringLiteral("en_US") << quint64(1048575) << QStringLiteral("1.0MB");
    QTest::newRow("smallest-MB") << QStringLiteral("en_US") << quint64(1048576) << QStringLiteral("1.0MB");
    QTest::newRow("largest-MB") << QStringLiteral("en_US") << quint64(1073689395) << QStringLiteral("1,023.9MB");
    QTest::newRow("rounds-to-GB") << QStringLiteral("en_US") << quint64(1073689396) << QStringLiteral("1.0GB");
    QTest::newRow("below-GB") << QStringLiteral("en_US") << quint64(1073741823) << QStringLiteral("1.0GB");
    QTest::newRow("smallest-GB") << QStringLiteral("en_US") << quint64(1073741824) << QStringLiteral("1.0GB");
    QTest::newRow("largest-GB") << QStringLiteral("en_US") << quint64(1099457940684) << QStringLiteral("1,023.9GB");
    QTest::newRow("rounds-to-TB") << QStringLiteral("en_US") << quint64(1099457940685) << QStringLiteral("1.0TB");
    QTest::newRow("below-TB") << QStringLiteral("en_US") << quint64(1099511627775) << QStringLiteral("1.0TB");
    QTest::newRow("smallest-TB") << QStringLiteral("en_US") << quint64(1099511627776) << QStringLiteral("1.0TB");
    QTest::newRow("largest-value") << QStringLiteral("en_US") << QGC_FORMAT_TEST_MAX
                                   << QStringLiteral("16,777,216.0TB");
    QTest::newRow("german-group-separator") << QStringLiteral("de_DE") << quint64(1023) << QStringLiteral("1.023B");
    QTest::newRow("german-decimal-comma") << QStringLiteral("de_DE") << quint64(1536) << QStringLiteral("1,5KB");
    QTest::newRow("c-locale-has-no-groups") << QStringLiteral("C") << quint64(1048524) << QStringLiteral("1023.9KB");
}

void QGCFormatTest::_bigSizeToString()
{
    QFETCH(QString, locale);
    QFETCH(quint64, value);
    QFETCH(QString, expected);

    const QGCFormatTestLocale defaultLocale(locale);
    QCOMPARE(QGC::bigSizeToString(value), expected);
}

void QGCFormatTest::_bigSizeMBToString_data()
{
    addQGCFormatTestColumns();
    QTest::newRow("zero") << QStringLiteral("en_US") << quint64(0) << QStringLiteral("0 MB");
    QTest::newRow("largest-MB") << QStringLiteral("en_US") << quint64(1023) << QStringLiteral("1,023 MB");
    QTest::newRow("smallest-GB") << QStringLiteral("en_US") << quint64(1024) << QStringLiteral("1.0 GB");
    QTest::newRow("rounds-down") << QStringLiteral("en_US") << quint64(1075) << QStringLiteral("1.0 GB");
    QTest::newRow("rounds-up") << QStringLiteral("en_US") << quint64(1076) << QStringLiteral("1.1 GB");
    QTest::newRow("largest-GB") << QStringLiteral("en_US") << quint64(1048524) << QStringLiteral("1,023.9 GB");
    QTest::newRow("rounds-to-TB") << QStringLiteral("en_US") << quint64(1048525) << QStringLiteral("1.00 TB");
    QTest::newRow("below-TB") << QStringLiteral("en_US") << quint64(1048575) << QStringLiteral("1.00 TB");
    QTest::newRow("smallest-TB") << QStringLiteral("en_US") << quint64(1048576) << QStringLiteral("1.00 TB");
    QTest::newRow("TB-has-two-decimals") << QStringLiteral("en_US") << quint64(1572864) << QStringLiteral("1.50 TB");
    QTest::newRow("largest-value") << QStringLiteral("en_US") << QGC_FORMAT_TEST_MAX
                                   << QStringLiteral("17,592,186,044,416.00 TB");
    QTest::newRow("german-decimal-comma") << QStringLiteral("de_DE") << quint64(1536) << QStringLiteral("1,5 GB");
    QTest::newRow("c-locale-has-no-groups") << QStringLiteral("C") << quint64(1048524) << QStringLiteral("1023.9 GB");
}

void QGCFormatTest::_bigSizeMBToString()
{
    QFETCH(QString, locale);
    QFETCH(quint64, value);
    QFETCH(QString, expected);

    const QGCFormatTestLocale defaultLocale(locale);
    QCOMPARE(QGC::bigSizeMBToString(value), expected);
}

QGC_REGISTER_PORTABLE_TEST(QGCFormatTest, TestLabel::Unit, TestLabel::Utilities)
