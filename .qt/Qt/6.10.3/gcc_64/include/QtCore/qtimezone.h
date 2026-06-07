// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIMEZONE_H
#define QTIMEZONE_H

#include <QtCore/qcompare.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlocale.h>
#include <QtCore/qswap.h>
#include <QtCore/qtclasshelpermacros.h>

#include <chrono>

#if QT_CONFIG(timezone) && (defined(Q_OS_DARWIN) || defined(Q_QDOC))
Q_FORWARD_DECLARE_CF_TYPE(CFTimeZone);
Q_FORWARD_DECLARE_OBJC_CLASS(NSTimeZone);
#endif

QT_BEGIN_NAMESPACE

class QTimeZonePrivate;

class Q_CORE_EXPORT QTimeZone
{
    struct ShortData
    {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        quintptr mode : 2;
#endif
        qintptr offset : sizeof(void *) * 8 - 2;

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        quintptr mode : 2;
#endif

        // mode is a cycled Qt::TimeSpec, (int(spec) + 1) % 4, so that zero
        // (lowest bits of a pointer) matches spec being Qt::TimeZone, for which
        // Data holds a QTZP pointer instead of ShortData.
        // Passing Qt::TimeZone gets the equivalent of a null QTZP; it is not short.
        constexpr ShortData(Qt::TimeSpec spec, int secondsAhead = 0)
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            : offset(spec == Qt::OffsetFromUTC ? secondsAhead : 0),
              mode((int(spec) + 1) & 3)
#else
            : mode((int(spec) + 1) & 3),
              offset(spec == Qt::OffsetFromUTC ? secondsAhead : 0)
#endif
        {
        }
        friend constexpr bool operator==(ShortData lhs, ShortData rhs)
        { return lhs.mode == rhs.mode && lhs.offset == rhs.offset; }
        constexpr Qt::TimeSpec spec() const { return Qt::TimeSpec((mode + 3) & 3); }
    };

    union Data
    {
        Data() noexcept;
        Data(ShortData sd) : s(sd) {}
        Data(const Data &other) noexcept;
        Data(Data &&other) noexcept : d(std::exchange(other.d, nullptr)) {}
        Data &operator=(const Data &other) noexcept;
        Data &operator=(Data &&other) noexcept { swap(other); return *this; }
        ~Data();

        void swap(Data &other) noexcept { qt_ptr_swap(d, other.d); }
        // isShort() is equivalent to s.spec() != Qt::TimeZone
        bool isShort() const { return s.mode; } // a.k.a. quintptr(d) & 3

        // Typse must support: out << wrap("C-strings");
        template <typename Stream, typename Wrap>
        void serialize(Stream &out, const Wrap &wrap) const;

        Data(QTimeZonePrivate *dptr) noexcept;
        Data &operator=(QTimeZonePrivate *dptr) noexcept;
        const QTimeZonePrivate *operator->() const { Q_ASSERT(!isShort()); return d; }
        QTimeZonePrivate *operator->() { Q_ASSERT(!isShort()); return d; }

        QTimeZonePrivate *d = nullptr;
        ShortData s;
    };
    QTimeZone(ShortData sd) : d(sd) {}
    QTimeZone(Qt::TimeSpec) Q_DECL_EQ_DELETE_X(
        "Would be treated as int offsetSeconds. "
        "Use QTimeZone::UTC or QTimeZone::LocalTime instead.");

public:
    // Sane UTC offsets range from -16 to +16 hours:
    static constexpr int MinUtcOffsetSecs = -16 * 3600;
    // No known modern zone > 12 hrs West of Greenwich.
    // Until 1844, Asia/Manila (in The Philippines) was at 15:56 West.
    static constexpr int MaxUtcOffsetSecs = +16 * 3600;
    // No known modern zone > 14 hrs East of Greenwich.
    // Until 1867, America/Metlakatla (in Alaska) was at 15:13:42 East.

    enum Initialization { LocalTime, UTC };

    QTimeZone() noexcept;
    Q_IMPLICIT QTimeZone(Initialization spec) noexcept
        : d(ShortData(spec == UTC ? Qt::UTC : Qt::LocalTime)) {}

#if QT_CONFIG(timezone)
    explicit QTimeZone(int offsetSeconds);
    explicit QTimeZone(const QByteArray &ianaId);
    QTimeZone(const QByteArray &zoneId, int offsetSeconds, const QString &name,
              const QString &abbreviation, QLocale::Territory territory = QLocale::AnyTerritory,
              const QString &comment = QString());
#endif // timezone backends

    QTimeZone(const QTimeZone &other) noexcept;
    QTimeZone(QTimeZone &&other) noexcept : d(std::move(other.d)) {}
    ~QTimeZone();

    QTimeZone &operator=(const QTimeZone &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QTimeZone)

    void swap(QTimeZone &other) noexcept
    { d.swap(other.d); }

#if QT_CORE_REMOVED_SINCE(6, 7)
    bool operator==(const QTimeZone &other) const;
    bool operator!=(const QTimeZone &other) const;
#endif

    bool isValid() const;

    static QTimeZone fromDurationAheadOfUtc(std::chrono::seconds offset)
    {
        return QTimeZone((offset.count() >= MinUtcOffsetSecs && offset.count() <= MaxUtcOffsetSecs)
                         ? ShortData(offset.count() ? Qt::OffsetFromUTC : Qt::UTC,
                                     int(offset.count()))
                         : ShortData(Qt::TimeZone));
    }
    static QTimeZone fromSecondsAheadOfUtc(int offset)
    {
        return fromDurationAheadOfUtc(std::chrono::seconds{offset});
    }
    constexpr Qt::TimeSpec timeSpec() const noexcept { return d.s.spec(); }
    constexpr int fixedSecondsAheadOfUtc() const noexcept
    { return timeSpec() == Qt::OffsetFromUTC ? int(d.s.offset) : 0; }

    static constexpr bool isUtcOrFixedOffset(Qt::TimeSpec spec) noexcept
    { return spec == Qt::UTC || spec == Qt::OffsetFromUTC; }
    constexpr bool isUtcOrFixedOffset() const noexcept { return isUtcOrFixedOffset(timeSpec()); }

#if QT_CONFIG(timezone)
    QTimeZone asBackendZone() const;

    enum TimeType {
        StandardTime = 0,
        DaylightTime = 1,
        GenericTime = 2
    };

    enum NameType {
        DefaultName = 0,
        LongName = 1,
        ShortName = 2,
        OffsetName = 3
    };

    struct OffsetData {
        QString abbreviation;
        QDateTime atUtc;
        int offsetFromUtc;
        int standardTimeOffset;
        int daylightTimeOffset;
    };
    typedef QList<OffsetData> OffsetDataList;

    bool hasAlternativeName(QByteArrayView alias) const;
    QByteArray id() const;
    QLocale::Territory territory() const;
#  if QT_DEPRECATED_SINCE(6, 6)
    QT_DEPRECATED_VERSION_X_6_6("Use territory() instead")
    QLocale::Country country() const;
#  endif
    QString comment() const;

    QString displayName(const QDateTime &atDateTime, NameType nameType = DefaultName,
                        const QLocale &locale = QLocale()) const;
    QString displayName(TimeType timeType, NameType nameType = DefaultName,
                        const QLocale &locale = QLocale()) const;
    QString abbreviation(const QDateTime &atDateTime) const;

    int offsetFromUtc(const QDateTime &atDateTime) const;
    int standardTimeOffset(const QDateTime &atDateTime) const;
    int daylightTimeOffset(const QDateTime &atDateTime) const;

    bool hasDaylightTime() const;
    bool isDaylightTime(const QDateTime &atDateTime) const;

    OffsetData offsetData(const QDateTime &forDateTime) const;

    bool hasTransitions() const;
    OffsetData nextTransition(const QDateTime &afterDateTime) const;
    OffsetData previousTransition(const QDateTime &beforeDateTime) const;
    OffsetDataList transitions(const QDateTime &fromDateTime, const QDateTime &toDateTime) const;

    static QByteArray systemTimeZoneId();
    static QTimeZone systemTimeZone();
    static QTimeZone utc();

    static bool isTimeZoneIdAvailable(const QByteArray &ianaId);

    static QList<QByteArray> availableTimeZoneIds();
    static QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory);
    static QList<QByteArray> availableTimeZoneIds(int offsetSeconds);

    static QByteArray ianaIdToWindowsId(const QByteArray &ianaId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                               QLocale::Territory territory);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId,
                                                QLocale::Territory territory);

#  if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    static QTimeZone fromCFTimeZone(CFTimeZoneRef timeZone);
    CFTimeZoneRef toCFTimeZone() const Q_DECL_CF_RETURNS_RETAINED;
    static QTimeZone fromNSTimeZone(const NSTimeZone *timeZone);
    NSTimeZone *toNSTimeZone() const Q_DECL_NS_RETURNS_AUTORELEASED;
#  endif

#  if __cpp_lib_chrono >= 201907L || defined(Q_QDOC)
    QT_POST_CXX17_API_IN_EXPORTED_CLASS
    static QTimeZone fromStdTimeZonePtr(const std::chrono::time_zone *timeZone)
    {
        if (!timeZone)
            return QTimeZone();
        const std::string_view timeZoneName = timeZone->name();
        return QTimeZone(QByteArrayView(timeZoneName).toByteArray());
    }
#  endif
#endif // feature timezone
private:
    friend Q_CORE_EXPORT bool comparesEqual(const QTimeZone &lhs, const QTimeZone &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QTimeZone)

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &ds, const QTimeZone &tz);
#endif
#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug dbg, const QTimeZone &tz);
#endif
    QTimeZone(QTimeZonePrivate &dd);
    friend class QTimeZonePrivate;
    friend class QDateTime;
    friend class QDateTimePrivate;
    Data d;
};

#if QT_CONFIG(timezone)
Q_DECLARE_TYPEINFO(QTimeZone::OffsetData, Q_RELOCATABLE_TYPE);
#endif
Q_DECLARE_SHARED(QTimeZone)

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &ds, const QTimeZone &tz);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &ds, QTimeZone &tz);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug dbg, const QTimeZone &tz);
#endif

#if QT_CONFIG(timezone) && __cpp_lib_chrono >= 201907L
// zoned_time
template <typename> // QT_POST_CXX17_API_IN_EXPORTED_CLASS
inline QDateTime QDateTime::fromStdZonedTime(const std::chrono::zoned_time<
                                                std::chrono::milliseconds,
                                                const std::chrono::time_zone *
                                             > &time)
{
    const auto sysTime = time.get_sys_time();
    const QTimeZone timeZone = QTimeZone::fromStdTimeZonePtr(time.get_time_zone());
    return fromMSecsSinceEpoch(sysTime.time_since_epoch().count(), timeZone);
}
#endif

QT_END_NAMESPACE

#endif // QTIMEZONE_H
