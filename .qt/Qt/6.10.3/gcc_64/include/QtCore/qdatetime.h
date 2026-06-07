// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDATETIME_H
#define QDATETIME_H

#include <QtCore/qcalendar.h>
#include <QtCore/qcompare.h>
#include <QtCore/qlocale.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

#include <climits>
#include <chrono>

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFDate);
Q_FORWARD_DECLARE_OBJC_CLASS(NSDate);
#endif

QT_BEGIN_NAMESPACE

class QTimeZone;
class QDateTime;

class Q_CORE_EXPORT QDate
{
    explicit constexpr QDate(qint64 julianDay) : jd(julianDay) {}
public:
    constexpr QDate() : jd(nullJd()) {}
    QDate(int y, int m, int d);
    QDate(int y, int m, int d, QCalendar cal);
// INTEGRITY incident-85878 (timezone and clock_cast are not supported)
#if (__cpp_lib_chrono >= 201907L && !defined(Q_OS_INTEGRITY)) || defined(Q_QDOC)
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    Q_IMPLICIT constexpr QDate(std::chrono::year_month_day date) noexcept
        : jd(date.ok() ? stdSysDaysToJulianDay(date) : nullJd())
    {}

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    Q_IMPLICIT constexpr QDate(std::chrono::year_month_day_last date) noexcept
        : jd(date.ok() ? stdSysDaysToJulianDay(date) : nullJd())
    {}

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    Q_IMPLICIT constexpr QDate(std::chrono::year_month_weekday date) noexcept
        : jd(date.ok() ? stdSysDaysToJulianDay(date) : nullJd())
    {}

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    Q_IMPLICIT constexpr QDate(std::chrono::year_month_weekday_last date) noexcept
        : jd(date.ok() ? stdSysDaysToJulianDay(date) : nullJd())
    {}

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    static constexpr QDate fromStdSysDays(const std::chrono::sys_days &days) noexcept
    {
        return QDate(stdSysDaysToJulianDay(days));
    }

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    constexpr std::chrono::sys_days toStdSysDays() const noexcept
    {
        const qint64 days = isValid() ? jd - unixEpochJd() : 0;
        return std::chrono::sys_days(std::chrono::days(days));
    }
#endif

    constexpr bool isNull() const { return !isValid(); }
    constexpr bool isValid() const { return jd >= minJd() && jd <= maxJd(); }

    // Gregorian-optimized:
    int year() const;
    int month() const;
    int day() const;
    int dayOfWeek() const;
    int dayOfYear() const;
    int daysInMonth() const;
    int daysInYear() const;
    int weekNumber(int *yearNum = nullptr) const; // ISO 8601, always Gregorian

    int year(QCalendar cal) const;
    int month(QCalendar cal) const;
    int day(QCalendar cal) const;
    int dayOfWeek(QCalendar cal) const;
    int dayOfYear(QCalendar cal) const;
    int daysInMonth(QCalendar cal) const;
    int daysInYear(QCalendar cal) const;

#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_VERSION_X_6_9("Pass QTimeZone instead")
    QDateTime startOfDay(Qt::TimeSpec spec, int offsetSeconds = 0) const;
    QT_DEPRECATED_VERSION_X_6_9("Pass QTimeZone instead")
    QDateTime endOfDay(Qt::TimeSpec spec, int offsetSeconds = 0) const;
#endif

    QDateTime startOfDay(const QTimeZone &zone) const;
    QDateTime endOfDay(const QTimeZone &zone) const;
    QDateTime startOfDay() const;
    QDateTime endOfDay() const;

#if QT_CONFIG(datestring)
    QString toString(Qt::DateFormat format = Qt::TextDate) const;
    QString toString(const QString &format) const;
    QString toString(const QString &format, QCalendar cal) const
    { return toString(qToStringViewIgnoringNull(format), cal); }
    QString toString(QStringView format) const;
    QString toString(QStringView format, QCalendar cal) const;
#endif
    bool setDate(int year, int month, int day); // Gregorian-optimized
    bool setDate(int year, int month, int day, QCalendar cal);

    void getDate(int *year, int *month, int *day) const;

    [[nodiscard]] QDate addDays(qint64 days) const;
// INTEGRITY incident-85878 (timezone and clock_cast are not supported)
#if (__cpp_lib_chrono >= 201907L && !defined(Q_OS_INTEGRITY)) || defined(Q_QDOC)
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    [[nodiscard]] QDate addDuration(std::chrono::days days) const
    {
        return addDays(days.count());
    }
#endif
    // Gregorian-optimized:
    [[nodiscard]] QDate addMonths(int months) const;
    [[nodiscard]] QDate addYears(int years) const;
    [[nodiscard]] QDate addMonths(int months, QCalendar cal) const;
    [[nodiscard]] QDate addYears(int years, QCalendar cal) const;
    qint64 daysTo(QDate d) const;

    static QDate currentDate();
#if QT_CONFIG(datestring)
    // No DateFormat accepts a two-digit year, so no need for baseYear:
    static QDate fromString(QStringView string, Qt::DateFormat format = Qt::TextDate);
    static QDate fromString(const QString &string, Qt::DateFormat format = Qt::TextDate)
    { return fromString(qToStringViewIgnoringNull(string), format); }

    // Accept calendar without over-ride of base year:
    static QDate fromString(QStringView string, QStringView format, QCalendar cal)
    { return fromString(string.toString(), format, QLocale::DefaultTwoDigitBaseYear, cal); }
    QT_CORE_INLINE_SINCE(6, 7)
    static QDate fromString(const QString &string, QStringView format, QCalendar cal);
    static QDate fromString(const QString &string, const QString &format, QCalendar cal)
    { return fromString(string, qToStringViewIgnoringNull(format), QLocale::DefaultTwoDigitBaseYear, cal); }

    // Overriding base year is likely more common than overriding calendar (and
    // likely to get more so, as the legacy base drops ever further behind us).
    static QDate fromString(QStringView string, QStringView format,
                            int baseYear = QLocale::DefaultTwoDigitBaseYear)
    { return fromString(string.toString(), format, baseYear); }
    static QDate fromString(QStringView string, QStringView format,
                            int baseYear, QCalendar cal)
    { return fromString(string.toString(), format, baseYear, cal); }
    static QDate fromString(const QString &string, QStringView format,
                            int baseYear = QLocale::DefaultTwoDigitBaseYear);
    static QDate fromString(const QString &string, QStringView format,
                            int baseYear, QCalendar cal);
    static QDate fromString(const QString &string, const QString &format,
                            int baseYear = QLocale::DefaultTwoDigitBaseYear)
    { return fromString(string, qToStringViewIgnoringNull(format), baseYear); }
    static QDate fromString(const QString &string, const QString &format,
                            int baseYear, QCalendar cal)
    { return fromString(string, qToStringViewIgnoringNull(format), baseYear, cal); }
#endif
    static bool isValid(int y, int m, int d);
    static bool isLeapYear(int year);

    static constexpr inline QDate fromJulianDay(qint64 jd_)
    { return jd_ >= minJd() && jd_ <= maxJd() ? QDate(jd_) : QDate() ; }
    constexpr inline qint64 toJulianDay() const { return jd; }

private:
    // using extra parentheses around min to avoid expanding it if it is a macro
    static constexpr inline qint64 nullJd() { return (std::numeric_limits<qint64>::min)(); }
    static constexpr inline qint64 minJd() { return Q_INT64_C(-784350574879); }
    static constexpr inline qint64 maxJd() { return Q_INT64_C( 784354017364); }
    static constexpr inline qint64 unixEpochJd() { return Q_INT64_C(2440588); }

// INTEGRITY incident-85878 (timezone and clock_cast are not supported)
#if __cpp_lib_chrono >= 201907L && !defined(Q_OS_INTEGRITY)
#if !QT_CORE_REMOVED_SINCE(6, 7)
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
#endif
    static constexpr qint64
    stdSysDaysToJulianDay(const std::chrono::sys_days &days) noexcept
    {
        const auto epochDays = days.time_since_epoch().count();
        // minJd() and maxJd() fit into 40 bits.
        if constexpr (sizeof(epochDays) * CHAR_BIT >= 41) {
            constexpr auto top = maxJd() - unixEpochJd();
            constexpr auto bottom = minJd() - unixEpochJd();
            if (epochDays > top || epochDays < bottom)
                return nullJd();
        }
        return unixEpochJd() + epochDays;
    }
#endif // __cpp_lib_chrono >= 201907L

    qint64 jd;

    friend class QDateTime;
    friend class QDateTimeParser;
    friend class QDateTimePrivate;

    friend constexpr bool comparesEqual(const QDate &lhs, const QDate &rhs) noexcept
    { return lhs.jd == rhs.jd; }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QDate &lhs, const QDate &rhs) noexcept
    { return Qt::compareThreeWay(lhs.jd, rhs.jd); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QDate)

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, QDate);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QDate &);
#endif
};
Q_DECLARE_TYPEINFO(QDate, Q_RELOCATABLE_TYPE);

class Q_CORE_EXPORT QTime
{
    explicit constexpr QTime(int ms) : mds(ms)
    {}
public:
    constexpr QTime(): mds(NullTime)
    {}
    QTime(int h, int m, int s = 0, int ms = 0);

    constexpr bool isNull() const { return mds == NullTime; }
    bool isValid() const;

    int hour() const;
    int minute() const;
    int second() const;
    int msec() const;
#if QT_CONFIG(datestring)
    QString toString(Qt::DateFormat f = Qt::TextDate) const;
    QString toString(const QString &format) const
    { return toString(qToStringViewIgnoringNull(format)); }
    QString toString(QStringView format) const;
#endif
    bool setHMS(int h, int m, int s, int ms = 0);

    [[nodiscard]] QTime addSecs(int secs) const;
    int secsTo(QTime t) const;
    [[nodiscard]] QTime addMSecs(int ms) const;
    int msecsTo(QTime t) const;

    static constexpr inline QTime fromMSecsSinceStartOfDay(int msecs) { return QTime(msecs); }
    constexpr inline int msecsSinceStartOfDay() const { return mds == NullTime ? 0 : mds; }

    static QTime currentTime();
#if QT_CONFIG(datestring)
    static QTime fromString(QStringView string, Qt::DateFormat format = Qt::TextDate);
    static QTime fromString(QStringView string, QStringView format)
    { return fromString(string.toString(), format); }
    static QTime fromString(const QString &string, QStringView format);
    static QTime fromString(const QString &string, Qt::DateFormat format = Qt::TextDate)
    { return fromString(qToStringViewIgnoringNull(string), format); }
    static QTime fromString(const QString &string, const QString &format)
    { return fromString(string, qToStringViewIgnoringNull(format)); }
#endif
    static bool isValid(int h, int m, int s, int ms = 0);

private:
    enum TimeFlag { NullTime = -1 };
    constexpr inline int ds() const { return mds == -1 ? 0 : mds; }
    int mds;

    friend constexpr bool comparesEqual(const QTime &lhs, const QTime &rhs) noexcept
    { return lhs.mds == rhs.mds; }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QTime &lhs, const QTime &rhs) noexcept
    { return Qt::compareThreeWay(lhs.mds, rhs.mds); }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QTime)

    friend class QDateTime;
    friend class QDateTimePrivate;
#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, QTime);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QTime &);
#endif
};
Q_DECLARE_TYPEINFO(QTime, Q_RELOCATABLE_TYPE);

class QDateTimePrivate;

class Q_CORE_EXPORT QDateTime
{
    struct ShortData {
#if QT_VERSION >= QT_VERSION_CHECK(7,0,0) || defined(QT_BOOTSTRAPPED)
#  if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qint64 status : 8;
#  endif
        qint64 msecs : 56;

#  if Q_BYTE_ORDER == Q_BIG_ENDIAN
        qint64 status : 8;
#  endif
#else
#  if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        quintptr status : 8;
#  endif
        // note: this is only 24 bits on 32-bit systems...
        qintptr msecs : sizeof(void *) * 8 - 8;

#  if Q_BYTE_ORDER == Q_BIG_ENDIAN
        quintptr status : 8;
#  endif
#endif
        friend constexpr bool operator==(ShortData lhs, ShortData rhs)
        { return lhs.status == rhs.status && lhs.msecs == rhs.msecs; }
    };

    union Data {
        // To be of any use, we need at least 60 years around 1970, which
        // is 1,893,456,000,000 ms. That requires 41 bits to store, plus
        // the sign bit. With the status byte, the minimum size is 50 bits.
        static constexpr bool CanBeSmall = sizeof(ShortData) * 8 > 50;

        Data() noexcept;
        Data(const QTimeZone &);
        Data(const Data &other) noexcept;
        Data(Data &&other) noexcept;
        Data &operator=(const Data &other) noexcept;
        Data &operator=(Data &&other) noexcept { swap(other); return *this; }
        ~Data();

        void swap(Data &other) noexcept
        { std::swap(data, other.data); }

        bool isShort() const;
        inline void invalidate();
        void detach();
        QTimeZone timeZone() const;

        const QDateTimePrivate *operator->() const;
        QDateTimePrivate *operator->();

        QDateTimePrivate *d;
        ShortData data;
    };

public:
    QDateTime() noexcept;

    enum class TransitionResolution {
        Reject = 0,
        RelativeToBefore,
        RelativeToAfter,
        PreferBefore,
        PreferAfter,
        PreferStandard,
        PreferDaylightSaving,
        // Closest match to behavior prior to introducing TransitionResolution:
        LegacyBehavior = RelativeToBefore
    };

#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_VERSION_X_6_9("Pass QTimeZone instead")
    QDateTime(QDate date, QTime time, Qt::TimeSpec spec, int offsetSeconds = 0);
#endif
#if QT_CORE_REMOVED_SINCE(6, 7)
    QDateTime(QDate date, QTime time, const QTimeZone &timeZone);
    QDateTime(QDate date, QTime time);
#endif
    QDateTime(QDate date, QTime time, const QTimeZone &timeZone,
              TransitionResolution resolve = TransitionResolution::LegacyBehavior);
    QDateTime(QDate date, QTime time,
              TransitionResolution resolve = TransitionResolution::LegacyBehavior);
    QDateTime(const QDateTime &other) noexcept;
    QDateTime(QDateTime &&other) noexcept;
    ~QDateTime();

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QDateTime)
    QDateTime &operator=(const QDateTime &other) noexcept;

    void swap(QDateTime &other) noexcept { d.swap(other.d); }

    bool isNull() const;
    bool isValid() const;

    QDate date() const;
    QTime time() const;
    Qt::TimeSpec timeSpec() const;
    int offsetFromUtc() const;
    QTimeZone timeRepresentation() const;
#if QT_CONFIG(timezone)
    QTimeZone timeZone() const;
#endif // timezone
    QString timeZoneAbbreviation() const;
    bool isDaylightTime() const;

    qint64 toMSecsSinceEpoch() const;
    qint64 toSecsSinceEpoch() const;

#if QT_CORE_REMOVED_SINCE(6, 7)
    void setDate(QDate date);
    void setTime(QTime time);
#endif
    void setDate(QDate date, TransitionResolution resolve = TransitionResolution::LegacyBehavior);
    void setTime(QTime time, TransitionResolution resolve = TransitionResolution::LegacyBehavior);

#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_VERSION_X_6_9("Use setTimeZone() instead")
    void setTimeSpec(Qt::TimeSpec spec);
    QT_DEPRECATED_VERSION_X_6_9("Use setTimeZone() instead")
    void setOffsetFromUtc(int offsetSeconds);
#endif
#if QT_CORE_REMOVED_SINCE(6, 7)
    void setTimeZone(const QTimeZone &toZone);
#endif
    void setTimeZone(const QTimeZone &toZone,
                     TransitionResolution resolve = TransitionResolution::LegacyBehavior);
    void setMSecsSinceEpoch(qint64 msecs);
    void setSecsSinceEpoch(qint64 secs);

#if QT_CONFIG(datestring)
    QString toString(Qt::DateFormat format = Qt::TextDate) const;
    QString toString(const QString &format) const;
    QString toString(const QString &format, QCalendar cal) const
    { return toString(qToStringViewIgnoringNull(format), cal); }
    QString toString(QStringView format) const;
    QString toString(QStringView format, QCalendar cal) const;
#endif
    [[nodiscard]] QDateTime addDays(qint64 days) const;
    [[nodiscard]] QDateTime addMonths(int months) const;
    [[nodiscard]] QDateTime addYears(int years) const;
    [[nodiscard]] QDateTime addSecs(qint64 secs) const;
    [[nodiscard]] QDateTime addMSecs(qint64 msecs) const;
    [[nodiscard]] QDateTime addDuration(std::chrono::milliseconds msecs) const
    {
        return addMSecs(msecs.count());
    }

#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_VERSION_X_6_9("Use toTimeZone instead")
    QDateTime toTimeSpec(Qt::TimeSpec spec) const;
#endif
    QDateTime toLocalTime() const;
    QDateTime toUTC() const;
    QDateTime toOffsetFromUtc(int offsetSeconds) const;
    QDateTime toTimeZone(const QTimeZone &toZone) const;

    qint64 daysTo(const QDateTime &) const;
    qint64 secsTo(const QDateTime &) const;
    qint64 msecsTo(const QDateTime &) const;

    static QDateTime currentDateTime(const QTimeZone &zone);
    static QDateTime currentDateTime();
    static QDateTime currentDateTimeUtc();
#if QT_CONFIG(datestring)
    // No DateFormat accepts a two-digit year, so no need for baseYear:
    static QDateTime fromString(QStringView string, Qt::DateFormat format = Qt::TextDate);
    static QDateTime fromString(const QString &string, Qt::DateFormat format = Qt::TextDate)
    { return fromString(qToStringViewIgnoringNull(string), format); }

    // Accept calendar without over-ride of base year:
    static QDateTime fromString(QStringView string, QStringView format, QCalendar cal)
    { return fromString(string.toString(), format, QLocale::DefaultTwoDigitBaseYear, cal); }
    QT_CORE_INLINE_SINCE(6, 7)
    static QDateTime fromString(const QString &string, QStringView format, QCalendar cal);
    static QDateTime fromString(const QString &string, const QString &format, QCalendar cal)
    { return fromString(string, qToStringViewIgnoringNull(format), QLocale::DefaultTwoDigitBaseYear, cal); }

    // Overriding base year is likely more common than overriding calendar (and
    // likely to get more so, as the legacy base drops ever further behind us).
    static QDateTime fromString(QStringView string, QStringView format,
                                int baseYear = QLocale::DefaultTwoDigitBaseYear)
    { return fromString(string.toString(), format, baseYear); }
    static QDateTime fromString(QStringView string, QStringView format,
                                int baseYear, QCalendar cal)
    { return fromString(string.toString(), format, baseYear, cal); }
    static QDateTime fromString(const QString &string, QStringView format,
                                int baseYear = QLocale::DefaultTwoDigitBaseYear);
    static QDateTime fromString(const QString &string, QStringView format,
                                int baseYear, QCalendar cal);
    static QDateTime fromString(const QString &string, const QString &format,
                                int baseYear = QLocale::DefaultTwoDigitBaseYear)
    { return fromString(string, qToStringViewIgnoringNull(format), baseYear); }
    static QDateTime fromString(const QString &string, const QString &format,
                                int baseYear, QCalendar cal)
    { return fromString(string, qToStringViewIgnoringNull(format), baseYear, cal); }
#endif

#if QT_DEPRECATED_SINCE(6, 9)
    QT_DEPRECATED_VERSION_X_6_9("Pass QTimeZone instead of time-spec, offset")
    static QDateTime fromMSecsSinceEpoch(qint64 msecs, Qt::TimeSpec spec, int offsetFromUtc = 0);
    QT_DEPRECATED_VERSION_X_6_9("Pass QTimeZone instead of time-spec, offset")
    static QDateTime fromSecsSinceEpoch(qint64 secs, Qt::TimeSpec spec, int offsetFromUtc = 0);
#endif

    static QDateTime fromMSecsSinceEpoch(qint64 msecs, const QTimeZone &timeZone);
    static QDateTime fromSecsSinceEpoch(qint64 secs, const QTimeZone &timeZone);
    static QDateTime fromMSecsSinceEpoch(qint64 msecs);
    static QDateTime fromSecsSinceEpoch(qint64 secs);

    static qint64 currentMSecsSinceEpoch() noexcept;
    static qint64 currentSecsSinceEpoch() noexcept;

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    static QDateTime fromCFDate(CFDateRef date);
    CFDateRef toCFDate() const Q_DECL_CF_RETURNS_RETAINED;
    static QDateTime fromNSDate(const NSDate *date);
    NSDate *toNSDate() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

    static QDateTime fromStdTimePoint(
        std::chrono::time_point<
            std::chrono::system_clock,
            std::chrono::milliseconds
        > time
    );

// INTEGRITY incident-85878 (timezone and clock_cast are not supported)
#if (__cpp_lib_chrono >= 201907L && !defined(Q_OS_INTEGRITY)) || defined(Q_QDOC)
#if __cpp_concepts >= 201907L || defined(Q_QDOC)
private:
    // The duration type of the result of a clock_cast<system_clock>.
    // This duration may differ from the duration of the input.
    template <typename Clock, typename Duration>
    using system_clock_cast_duration = decltype(
        std::chrono::clock_cast<std::chrono::system_clock>(
            std::declval<const std::chrono::time_point<Clock, Duration> &>()
        ).time_since_epoch()
    );

public:
    // Generic clock, as long as it's compatible with us (= system_clock)
    template <typename Clock, typename Duration>
    static QDateTime fromStdTimePoint(const std::chrono::time_point<Clock, Duration> &time)
        requires
            requires(const std::chrono::time_point<Clock, Duration> &t) {
                // the clock can be converted to system_clock
                std::chrono::clock_cast<std::chrono::system_clock>(t);
                // after the conversion to system_clock, the duration type
                // we get is convertible to milliseconds
                requires std::is_convertible_v<
                    system_clock_cast_duration<Clock, Duration>,
                    std::chrono::milliseconds
                >;
            }
    {
        using namespace std::chrono;
        const sys_time<milliseconds> sysTime = clock_cast<system_clock>(time);
        return fromStdTimePoint(sysTime);
    }
#endif // __cpp_concepts

    // local_time
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    static QDateTime fromStdTimePoint(const std::chrono::local_time<std::chrono::milliseconds> &time)
    {
        return fromStdLocalTime(time);
    }

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    static QDateTime fromStdLocalTime(const std::chrono::local_time<std::chrono::milliseconds> &time)
    {
        QDateTime result(QDate(1970, 1, 1), QTime(0, 0, 0), TransitionResolution::LegacyBehavior);
        return result.addMSecs(time.time_since_epoch().count());
    }

#if QT_CONFIG(timezone) && (__cpp_lib_chrono >= 201907L || defined(Q_QDOC))
    // zoned_time. defined in qtimezone.h
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    static QDateTime fromStdZonedTime(const std::chrono::zoned_time<
                                          std::chrono::milliseconds,
                                          const std::chrono::time_zone *
                                      > &time);
#endif // QT_CONFIG(timezone)

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    std::chrono::sys_time<std::chrono::milliseconds> toStdSysMilliseconds() const
    {
        const std::chrono::milliseconds duration(toMSecsSinceEpoch());
        return std::chrono::sys_time<std::chrono::milliseconds>(duration);
    }

    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    std::chrono::sys_seconds toStdSysSeconds() const
    {
        const std::chrono::seconds duration(toSecsSinceEpoch());
        return std::chrono::sys_seconds(duration);
    }
#endif // __cpp_lib_chrono >= 201907L && !Q_OS_INTEGRITY

    friend std::chrono::milliseconds operator-(const QDateTime &lhs, const QDateTime &rhs)
    {
        return std::chrono::milliseconds(rhs.msecsTo(lhs));
    }

    friend QDateTime operator+(const QDateTime &dateTime, std::chrono::milliseconds duration)
    {
        return dateTime.addMSecs(duration.count());
    }

    friend QDateTime operator+(std::chrono::milliseconds duration, const QDateTime &dateTime)
    {
        return dateTime + duration;
    }

    QDateTime &operator+=(std::chrono::milliseconds duration)
    {
        *this = addMSecs(duration.count());
        return *this;
    }

    friend QDateTime operator-(const QDateTime &dateTime, std::chrono::milliseconds duration)
    {
        return dateTime.addMSecs(-duration.count());
    }

    QDateTime &operator-=(std::chrono::milliseconds duration)
    {
        *this = addMSecs(-duration.count());
        return *this;
    }

    // (1<<63) ms is 292277024.6 (average Gregorian) years, counted from the start of 1970, so
    // Last is floor(1970 + 292277024.6); no year 0, so First is floor(1970 - 1 - 292277024.6)
    enum class YearRange : qint32 { First = -292275056,  Last = +292278994 };

private:
    bool equals(const QDateTime &other) const;
#if QT_CORE_REMOVED_SINCE(6, 7)
    bool precedes(const QDateTime &other) const;
#endif
    friend class QDateTimePrivate;

    Data d;

    friend bool comparesEqual(const QDateTime &lhs, const QDateTime &rhs)
    { return lhs.equals(rhs); }
    friend Q_CORE_EXPORT Qt::weak_ordering
    compareThreeWay(const QDateTime &lhs, const QDateTime &rhs);
    Q_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT(QDateTime)

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QDateTime &);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QDateTime &);
#endif

#if !defined(QT_NO_DEBUG_STREAM) && QT_CONFIG(datestring)
    friend Q_CORE_EXPORT QDebug operator<<(QDebug, const QDateTime &);
#endif
};
Q_DECLARE_SHARED(QDateTime)

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, QDate);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QDate &);
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, QTime);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QTime &);
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QDateTime &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QDateTime &);
#endif // QT_NO_DATASTREAM

#if !defined(QT_NO_DEBUG_STREAM) && QT_CONFIG(datestring)
Q_CORE_EXPORT QDebug operator<<(QDebug, QDate);
Q_CORE_EXPORT QDebug operator<<(QDebug, QTime);
Q_CORE_EXPORT QDebug operator<<(QDebug, const QDateTime &);
#endif

// QDateTime is not noexcept for now -- to be revised once
// timezone and calendaring support is added
Q_CORE_EXPORT size_t qHash(const QDateTime &key, size_t seed = 0);
Q_CORE_EXPORT size_t qHash(QDate key, size_t seed = 0) noexcept;
Q_CORE_EXPORT size_t qHash(QTime key, size_t seed = 0) noexcept;

#if QT_CONFIG(datestring) && QT_CORE_INLINE_IMPL_SINCE(6, 7)
QDate QDate::fromString(const QString &string, QStringView format, QCalendar cal)
{
    return fromString(string, format, QLocale::DefaultTwoDigitBaseYear, cal);
}

QDateTime QDateTime::fromString(const QString &string, QStringView format, QCalendar cal)
{
    return fromString(string, format, QLocale::DefaultTwoDigitBaseYear, cal);
}
#endif

QT_END_NAMESPACE

#endif // QDATETIME_H
