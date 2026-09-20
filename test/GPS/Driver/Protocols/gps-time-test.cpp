#include "GPSProtocolTime.h"
#include "UnitTest.h"

Q_DECLARE_METATYPE(tm)

static tm calendar(int year, int month, int day, int hour, int minute, int second, int isdst = 0)
{
    tm utc{};
    utc.tm_year = year - 1900;
    utc.tm_mon = month - 1;
    utc.tm_mday = day;
    utc.tm_hour = hour;
    utc.tm_min = minute;
    utc.tm_sec = second;
    utc.tm_isdst = isdst;
    return utc;
}

class GPSProtocolUtcTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _utc_data();
    void _utc();
};

void GPSProtocolUtcTest::_utc_data()
{
    QTest::addColumn<tm>("utc");
    QTest::addColumn<qint64>("epoch");

    const struct
    {
        const char* name;
        tm utc;
        int64_t expected;
    } cases[] = {
        {"receiver-fix", calendar(2026, 9, 8, 15, 58, 9), 1788883089},
        {"winter", calendar(2026, 1, 8, 15, 58, 9), 1767887889},
        {"dst-gap", calendar(2026, 3, 8, 2, 30, 0, -1), 1772937000},
        {"dst-overlap", calendar(2026, 11, 1, 1, 30, 0, -1), 1793496600},
        {"ignore-dst-flag", calendar(2026, 9, 8, 15, 58, 9, 1), 1788883089},
        {"leap-day", calendar(2024, 2, 29, 23, 59, 59), 1709251199},
        {"second-overflow", calendar(2024, 2, 29, 23, 59, 60), 1709251200},
        {"sbf-gps-week", calendar(1980, 1, 6 + 2435 * 7, 0, 0, 2 * 86400 + 15 * 3600 + 58 * 60 + 9), 1788883089},
        {"leap-century", calendar(2000, 2, 29, 23, 59, 60), 951868800},
        {"non-leap-century", calendar(2100, 2, 29, 0, 0, 0), 4107542400LL},
        {"month-underflow", calendar(2026, 0, 1, 0, 0, 0), 1764547200},
        {"month-overflow", calendar(2026, 13, 1, 0, 0, 0), 1798761600},
        {"day-underflow", calendar(2024, 3, 0, 0, 0, 0), 1709164800},
        {"second-underflow", calendar(2024, 1, 1, 0, 0, -1), 1704067199},
        {"signed-32-bit-limit", calendar(2038, 1, 19, 3, 14, 7), 2147483647},
        {"signed-32-bit-overflow", calendar(2038, 1, 19, 3, 14, 8), 2147483648LL},
        {"signed-32-bit-next-day", calendar(2038, 1, 20, 0, 0, 0), 2147558400LL},
    };

    for (const auto& test : cases) {
        QTest::newRow(test.name) << test.utc << qint64(test.expected);
    }
}

void GPSProtocolUtcTest::_utc()
{
    QFETCH(tm, utc);
    QFETCH(qint64, epoch);
    const time_t actual = gpsTimeToEpoch(utc);
    const bool representable =
        static_cast<uint64_t>(epoch) <= static_cast<uint64_t>((std::numeric_limits<time_t>::max)());
    const time_t expected = representable ? static_cast<time_t>(epoch) : static_cast<time_t>(-1);
    QCOMPARE(actual, expected);
    if (!representable) {
        return;
    }

    const tm* normalized = gmtime(&expected);
    QVERIFY(normalized);
    QCOMPARE(utc.tm_year, normalized->tm_year);
    QCOMPARE(utc.tm_mon, normalized->tm_mon);
    QCOMPARE(utc.tm_mday, normalized->tm_mday);
    QCOMPARE(utc.tm_hour, normalized->tm_hour);
    QCOMPARE(utc.tm_min, normalized->tm_min);
    QCOMPARE(utc.tm_sec, normalized->tm_sec);
    QCOMPARE(utc.tm_yday, normalized->tm_yday);
    QCOMPARE(utc.tm_wday, normalized->tm_wday);
    QCOMPARE(utc.tm_isdst, 0);
    if (qstrcmp(QTest::currentDataTag(), "sbf-gps-week") == 0) {
        QCOMPARE(utc.tm_year, 126);
        QCOMPARE(utc.tm_mon, 8);
        QCOMPARE(utc.tm_mday, 8);
        QCOMPARE(utc.tm_hour, 15);
        QCOMPARE(utc.tm_min, 58);
        QCOMPARE(utc.tm_sec, 9);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolUtcTest, TestLabel::Unit)

#include "gps-time-test.moc"
