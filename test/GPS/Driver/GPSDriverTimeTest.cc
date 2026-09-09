#include "GPSDriverTimeTest.h"

#include <QtCore/QDateTime>

#include <gps_helper.h>
#include <gps_time.h>

namespace {

class TimeConversionDriver : public GPSHelper
{
public:
    TimeConversionDriver()
        : GPSHelper(&_callback, this)
    {}

    int configure(unsigned&, const GPSConfig&) override { return 0; }

    int receive(unsigned) override { return 0; }

    using GPSHelper::timeFromUtc;

    int clockUpdates = 0;
    timespec clockTime{};

private:
    static int _callback(GPSCallbackType type, void* data, int, void* user)
    {
        auto* driver = static_cast<TimeConversionDriver*>(user);
        if (type == GPSCallbackType::setClock) {
            ++driver->clockUpdates;
            driver->clockTime = *static_cast<timespec*>(data);
        }
        return 0;
    }
};

tm calendarFields(const QDateTime& dateTime, int daylightSaving)
{
    tm utc{};
    utc.tm_year = dateTime.date().year() - 1900;
    utc.tm_mon = dateTime.date().month() - 1;
    utc.tm_mday = dateTime.date().day();
    utc.tm_hour = dateTime.time().hour();
    utc.tm_min = dateTime.time().minute();
    utc.tm_sec = dateTime.time().second();
    utc.tm_isdst = daylightSaving;
    return utc;
}

}  // namespace

void GPSDriverTimeTest::_convertsUtc_data()
{
    QTest::addColumn<QString>("isoTime");
    QTest::addColumn<int>("nanoseconds");
    QTest::addColumn<int>("daylightSaving");

    QTest::newRow("observed-receiver-fix") << QStringLiteral("2026-09-08T15:58:09Z") << 800000000 << 0;
    QTest::newRow("winter") << QStringLiteral("2026-01-08T15:58:09Z") << 0 << 0;
    QTest::newRow("dst-gap") << QStringLiteral("2026-03-08T02:30:00Z") << 0 << -1;
    QTest::newRow("dst-overlap") << QStringLiteral("2026-11-01T01:30:00Z") << 0 << -1;
    QTest::newRow("ignore-dst-flag") << QStringLiteral("2026-09-08T15:58:09Z") << 0 << 1;
    QTest::newRow("negative-fraction") << QStringLiteral("2026-09-08T00:00:00Z") << -200000000 << 0;
    QTest::newRow("leap-day") << QStringLiteral("2024-02-29T23:59:59Z") << 999999000 << 0;
}

void GPSDriverTimeTest::_convertsUtc()
{
    QFETCH(QString, isoTime);
    QFETCH(int, nanoseconds);
    QFETCH(int, daylightSaving);

    const QDateTime expected = QDateTime::fromString(isoTime, Qt::ISODate);
    QVERIFY(expected.isValid());
    auto utc = calendarFields(expected, daylightSaving);
    TimeConversionDriver driver;
    const auto expectedUsecs = static_cast<uint64_t>(expected.toSecsSinceEpoch() * 1000000 + nanoseconds / 1000);

    QCOMPARE(driver.timeFromUtc(utc, nanoseconds), expectedUsecs);
    QCOMPARE(driver.clockUpdates, 1);
    QCOMPARE(driver.clockTime.tv_sec, expected.toSecsSinceEpoch());
    QCOMPARE(driver.clockTime.tv_nsec, nanoseconds);

    utc = calendarFields(expected, daylightSaving);
    QCOMPARE(driver.timeFromUtc(utc, nanoseconds, false), expectedUsecs);
    QCOMPARE(driver.clockUpdates, 1);
}

void GPSDriverTimeTest::_normalizesGpsWeek()
{
    // SBF represents GPS week and time-of-week as overflowing January 1980 calendar fields.
    tm utc{};
    utc.tm_year = 1980 - 1900;
    utc.tm_mon = 0;
    utc.tm_mday = 6 + 2435 * 7;
    utc.tm_sec = 2 * 86400 + 15 * 3600 + 58 * 60 + 9;

    const QDateTime expected = QDateTime::fromString(QStringLiteral("2026-09-08T15:58:09Z"), Qt::ISODate);
    QCOMPARE(gpsTimeToEpoch(utc), expected.toSecsSinceEpoch());
    QCOMPARE(utc.tm_year, 2026 - 1900);
    QCOMPARE(utc.tm_mon, 8);
    QCOMPARE(utc.tm_mday, 8);
    QCOMPARE(utc.tm_hour, 15);
    QCOMPARE(utc.tm_min, 58);
    QCOMPARE(utc.tm_sec, 9);
}

void GPSDriverTimeTest::_rejectsInvalidEpoch()
{
    TimeConversionDriver driver;
    const QDateTime oldTime = QDateTime::fromString(QStringLiteral("1980-01-01T00:00:00Z"), Qt::ISODate);
    auto utc = calendarFields(oldTime, 0);
    QCOMPARE(driver.timeFromUtc(utc, 0), uint64_t{0});
    QCOMPARE(driver.clockUpdates, 0);
}

UT_REGISTER_TEST(GPSDriverTimeTest, TestLabel::Unit)
