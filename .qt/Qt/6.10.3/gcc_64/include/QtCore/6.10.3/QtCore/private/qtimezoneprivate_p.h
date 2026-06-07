// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QTIMEZONEPRIVATE_P_H
#define QTIMEZONEPRIVATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlist.h"
#include "qtimezone.h"
#include "private/qlocale_p.h"
#include "private/qdatetime_p.h"

#if QT_CONFIG(timezone_tzdb)
#include <chrono>
#endif
#include <optional>

#if QT_CONFIG(icu)
#include <unicode/ucal.h>
#endif

#ifdef Q_OS_DARWIN
Q_FORWARD_DECLARE_OBJC_CLASS(NSTimeZone);
#endif // Q_OS_DARWIN

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif // Q_OS_WIN

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

QT_REQUIRE_CONFIG(timezone);
QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QTimeZonePrivate : public QSharedData
{
    // Nothing should be copy-assigning instances of either this or its derived
    // classes (only clone() should copy, using the copy-constructor):
    QTimeZonePrivate &operator=(const QTimeZonePrivate &) const = delete;
protected:
    QTimeZonePrivate(const QTimeZonePrivate &other) = default;
public:
    // Version of QTimeZone::OffsetData struct using msecs for efficiency
    struct Data {
        QString abbreviation;
        qint64 atMSecsSinceEpoch;
        int offsetFromUtc;
        int standardTimeOffset;
        int daylightTimeOffset;
        Data()
            : atMSecsSinceEpoch(QTimeZonePrivate::invalidMSecs()),
              offsetFromUtc(QTimeZonePrivate::invalidSeconds()),
              standardTimeOffset(QTimeZonePrivate::invalidSeconds()),
              daylightTimeOffset(QTimeZonePrivate::invalidSeconds())
        {}
        Data(const QString &name, qint64 when, int offset, int standard)
            : abbreviation(name),
              atMSecsSinceEpoch(when),
              offsetFromUtc(offset),
              standardTimeOffset(standard),
              daylightTimeOffset(offset - standard)
        {}
    };
    typedef QList<Data> DataList;

    // Create null time zone
    QTimeZonePrivate();
    virtual ~QTimeZonePrivate();

    virtual QTimeZonePrivate *clone() const = 0;

    bool operator==(const QTimeZonePrivate &other) const;
    bool operator!=(const QTimeZonePrivate &other) const;

    bool isValid() const;

    QByteArray id() const;
    virtual QLocale::Territory territory() const;
    virtual QString comment() const;

    virtual QString displayName(qint64 atMSecsSinceEpoch,
                                QTimeZone::NameType nameType,
                                const QLocale &locale) const;
    virtual QString displayName(QTimeZone::TimeType timeType,
                                QTimeZone::NameType nameType,
                                const QLocale &locale) const;
    virtual QString abbreviation(qint64 atMSecsSinceEpoch) const;

    virtual int offsetFromUtc(qint64 atMSecsSinceEpoch) const;
    virtual int standardTimeOffset(qint64 atMSecsSinceEpoch) const;
    virtual int daylightTimeOffset(qint64 atMSecsSinceEpoch) const;

    virtual bool hasDaylightTime() const;
    virtual bool isDaylightTime(qint64 atMSecsSinceEpoch) const;

    virtual Data data(qint64 forMSecsSinceEpoch) const;
    virtual Data data(QTimeZone::TimeType timeType) const;
    virtual bool isDataLocale(const QLocale &locale) const;
    static bool isAnglicLocale(const QLocale &locale)
    {
        // Sufficiently like the C locale for displayName()-related purposes:
        const QLocale::Language lang = locale.language();
        return lang == QLocale::C
            || (lang == QLocale::English && locale.script() == QLocale::LatinScript);
    }
    QDateTimePrivate::ZoneState stateAtZoneTime(qint64 forLocalMSecs,
                                                QDateTimePrivate::TransitionOptions resolve) const;

    virtual bool hasTransitions() const;
    virtual Data nextTransition(qint64 afterMSecsSinceEpoch) const;
    virtual Data previousTransition(qint64 beforeMSecsSinceEpoch) const;
    DataList transitions(qint64 fromMSecsSinceEpoch, qint64 toMSecsSinceEpoch) const;

    virtual QByteArray systemTimeZoneId() const;

    virtual bool isTimeZoneIdAvailable(const QByteArray &ianaId) const;
    virtual QList<QByteArray> availableTimeZoneIds() const = 0;
    virtual QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory) const;
    virtual QList<QByteArray> availableTimeZoneIds(int utcOffset) const;

    virtual void serialize(QDataStream &ds) const;

    // Static Utility Methods
    [[nodiscard]] static constexpr qint64 maxMSecs()
    { return (std::numeric_limits<qint64>::max)(); }
    [[nodiscard]] static constexpr qint64 minMSecs()
    { return (std::numeric_limits<qint64>::min)() + 1; }
    [[nodiscard]] static constexpr qint64 invalidMSecs()
    { return (std::numeric_limits<qint64>::min)(); }
    [[nodiscard]] static constexpr int invalidSeconds()
    { return (std::numeric_limits<int>::min)(); }
    static QTimeZone::OffsetData invalidOffsetData();
    static QTimeZone::OffsetData toOffsetData(const Data &data);
    static bool isValidId(const QByteArray &ianaId);
    static QString isoOffsetFormat(int offsetFromUtc,
                                   QTimeZone::NameType mode = QTimeZone::OffsetName);

    static QByteArray aliasToIana(QByteArrayView alias);
    static QByteArray ianaIdToWindowsId(const QByteArray &ianaId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                               QLocale::Territory territory);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId,
                                                QLocale::Territory territory);
    struct NamePrefixMatch
    {
        QByteArray ianaId;
        qsizetype nameLength = 0;
        QTimeZone::TimeType timeType = QTimeZone::GenericTime;
    };
    static NamePrefixMatch findLongNamePrefix(QStringView text, const QLocale &locale,
                                              std::optional<qint64> atEpochMillis = std::nullopt);

    // returns "UTC" QString and QByteArray
    [[nodiscard]] static inline QString utcQString()
    {
        return QStringLiteral("UTC");
    }

    [[nodiscard]] static inline QByteArray utcQByteArray()
    {
        return QByteArrayLiteral("UTC");
    }

    [[nodiscard]] static QTimeZone utcQTimeZone();

#ifdef QT_BUILD_INTERNAL // For the benefit of a test
    [[nodiscard]] static inline const QTimeZonePrivate *extractPrivate(const QTimeZone &zone)
    {
        return zone.d.operator->();
    }
#endif

protected:
    // Zones CLDR data says match a condition.
    // Use to filter what the backend has available.
    QList<QByteArrayView> matchingTimeZoneIds(QLocale::Territory territory) const;
    QList<QByteArrayView> matchingTimeZoneIds(int utcOffset) const;

#if QT_CONFIG(timezone_locale)
    // Defined in qtimezonelocale.cpp
    QString localeName(qint64 atMSecsSinceEpoch, int offsetFromUtc,
                       QTimeZone::TimeType timeType,
                       QTimeZone::NameType nameType,
                       const QLocale &locale) const;
#endif // L10n helpers.

    QByteArray m_id;
};
Q_DECLARE_TYPEINFO(QTimeZonePrivate::Data, Q_RELOCATABLE_TYPE);

class Q_AUTOTEST_EXPORT QUtcTimeZonePrivate final : public QTimeZonePrivate
{
    QUtcTimeZonePrivate &operator=(const QUtcTimeZonePrivate &) const = delete;
    QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other);
public:
    // Create default UTC time zone
    QUtcTimeZonePrivate();
    // Create named time zone
    QUtcTimeZonePrivate(const QByteArray &utcId);
    // Create offset from UTC
    QUtcTimeZonePrivate(qint32 offsetSeconds);
    // Create custom offset from UTC
    QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                        const QString &abbreviation, QLocale::Territory territory,
                        const QString &comment);
    virtual ~QUtcTimeZonePrivate();

    // Fall-back for UTC[+-]\d+(:\d+){,2} IDs.
    static qint64 offsetFromUtcString(QByteArrayView id);

    QUtcTimeZonePrivate *clone() const override;

    Data data(qint64 forMSecsSinceEpoch) const override;
    Data data(QTimeZone::TimeType timeType) const override;
    bool isDataLocale(const QLocale &locale) const override;

    QLocale::Territory territory() const override;
    QString comment() const override;

    QString displayName(qint64 atMSecsSinceEpoch,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(QLocale::Territory country) const override;
    QList<QByteArray> availableTimeZoneIds(int utcOffset) const override;

    void serialize(QDataStream &ds) const override;

private:
    void init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
              const QString &abbreviation, QLocale::Territory territory,
              const QString &comment);

    QString m_name;
    QString m_abbreviation;
    QString m_comment;
    QLocale::Territory m_territory;
    int m_offsetFromUtc;
};

// Platform backend cascade: match newBackendTimeZone() in qtimezone.cpp
#if QT_CONFIG(timezone_tzdb)
class QChronoTimeZonePrivate final : public QTimeZonePrivate
{
    QChronoTimeZonePrivate &operator=(const QChronoTimeZonePrivate &) const = delete;
    QChronoTimeZonePrivate(const QChronoTimeZonePrivate &) = default;
public:
    QChronoTimeZonePrivate();
    QChronoTimeZonePrivate(QByteArrayView id);
    ~QChronoTimeZonePrivate() override;
    QChronoTimeZonePrivate *clone() const override;

    QByteArray systemTimeZoneId() const override;

    QString abbreviation(qint64 atMSecsSinceEpoch) const override;
    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(int utcOffset) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

private:
    const std::chrono::time_zone *const m_timeZone;
};
#elif defined(Q_OS_DARWIN)
class Q_AUTOTEST_EXPORT QMacTimeZonePrivate final : public QTimeZonePrivate
{
    QMacTimeZonePrivate &operator=(const QMacTimeZonePrivate &) const = delete;
    QMacTimeZonePrivate(const QMacTimeZonePrivate &other);
public:
    // Create default time zone
    QMacTimeZonePrivate();
    // Create named time zone
    QMacTimeZonePrivate(const QByteArray &ianaId);
    ~QMacTimeZonePrivate();

    QMacTimeZonePrivate *clone() const override;

    QString comment() const override;

    using QTimeZonePrivate::displayName;
    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    using QTimeZonePrivate::data;
    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;
    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;

    NSTimeZone *nsTimeZone() const;

private:
    void init(const QByteArray &zoneId);

    NSTimeZone *m_nstz;
};
#elif defined(Q_OS_ANDROID)
class QAndroidTimeZonePrivate final : public QTimeZonePrivate
{
    QAndroidTimeZonePrivate &operator=(const QAndroidTimeZonePrivate &) const = delete;
    QAndroidTimeZonePrivate(const QAndroidTimeZonePrivate &) = default;
public:
    // Create default time zone
    QAndroidTimeZonePrivate();
    // Create named time zone
    QAndroidTimeZonePrivate(const QByteArray &ianaId);
    ~QAndroidTimeZonePrivate();

    QAndroidTimeZonePrivate *clone() const override;

    using QTimeZonePrivate::displayName;
    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    using QTimeZonePrivate::data;
    Data data(qint64 forMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;
    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;

private:
    void init(const QByteArray &zoneId);

    QJniObject androidTimeZone;
};
#elif defined(Q_OS_UNIX) && !defined(Q_OS_VXWORKS)
struct QTzTransitionTime
{
    qint64 atMSecsSinceEpoch;
    quint8 ruleIndex;
};
Q_DECLARE_TYPEINFO(QTzTransitionTime, Q_PRIMITIVE_TYPE);
struct QTzTransitionRule
{
    int stdOffset = 0;
    int dstOffset = 0;
    quint8 abbreviationIndex = 0;
};
Q_DECLARE_TYPEINFO(QTzTransitionRule, Q_PRIMITIVE_TYPE);
constexpr inline bool operator==(const QTzTransitionRule &lhs, const QTzTransitionRule &rhs) noexcept
{ return lhs.stdOffset == rhs.stdOffset && lhs.dstOffset == rhs.dstOffset && lhs.abbreviationIndex == rhs.abbreviationIndex; }
constexpr inline bool operator!=(const QTzTransitionRule &lhs, const QTzTransitionRule &rhs) noexcept
{ return !operator==(lhs, rhs); }

// These are stored separately from QTzTimeZonePrivate so that they can be
// cached, avoiding the need to re-parse them from disk constantly.
struct QTzTimeZoneCacheEntry
{
    QList<QTzTransitionTime> m_tranTimes;
    QList<QTzTransitionRule> m_tranRules;
    QList<QByteArray> m_abbreviations;
    QByteArray m_posixRule;
    QTzTransitionRule m_preZoneRule;
    bool m_hasDst = false;
};

class Q_AUTOTEST_EXPORT QTzTimeZonePrivate final : public QTimeZonePrivate
{
    QTzTimeZonePrivate &operator=(const QTzTimeZonePrivate &) const = delete;
    QTzTimeZonePrivate(const QTzTimeZonePrivate &) = default;
public:
    // Create default time zone
    QTzTimeZonePrivate();
    // Create named time zone
    QTzTimeZonePrivate(const QByteArray &ianaId);
    ~QTzTimeZonePrivate();

    QTzTimeZonePrivate *clone() const override;

    QLocale::Territory territory() const override;
    QString comment() const override;

    using QTimeZonePrivate::displayName;
    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    Data data(qint64 forMSecsSinceEpoch) const override;
    Data data(QTimeZone::TimeType timeType) const override;
    bool isDataLocale(const QLocale &locale) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory) const override;

private:
    static QByteArray staticSystemTimeZoneId();
    QList<QTimeZonePrivate::Data> getPosixTransitions(qint64 msNear) const;

    Data dataForTzTransition(QTzTransitionTime tran) const;
    Data dataFromRule(QTzTransitionRule rule, qint64 msecsSinceEpoch) const;
    QTzTimeZoneCacheEntry cached_data;
    const QList<QTzTransitionTime> &tranCache() const { return cached_data.m_tranTimes; }
};
#elif QT_CONFIG(icu)
class Q_AUTOTEST_EXPORT QIcuTimeZonePrivate final : public QTimeZonePrivate
{
    QIcuTimeZonePrivate &operator=(const QIcuTimeZonePrivate &) const = delete;
    QIcuTimeZonePrivate(const QIcuTimeZonePrivate &other);
public:
    // Create default time zone
    QIcuTimeZonePrivate();
    // Create named time zone
    QIcuTimeZonePrivate(const QByteArray &ianaId);
    ~QIcuTimeZonePrivate();

    QIcuTimeZonePrivate *clone() const override;

    using QTimeZonePrivate::displayName;
    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    using QTimeZonePrivate::data;
    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    bool isTimeZoneIdAvailable(const QByteArray &ianaId) const override;
    QList<QByteArray> availableTimeZoneIds() const override;
    QList<QByteArray> availableTimeZoneIds(QLocale::Territory territory) const override;
    QList<QByteArray> availableTimeZoneIds(int offsetFromUtc) const override;

private:
    void init(const QByteArray &ianaId);

    UCalendar *m_ucal;
};
#elif defined(Q_OS_WIN)
class Q_AUTOTEST_EXPORT QWinTimeZonePrivate final : public QTimeZonePrivate
{
    QWinTimeZonePrivate &operator=(const QWinTimeZonePrivate &) const = delete;
    QWinTimeZonePrivate(const QWinTimeZonePrivate &) = default;
public:
    struct QWinTransitionRule {
        int startYear;
        int standardTimeBias;
        int daylightTimeBias;
        SYSTEMTIME standardTimeRule;
        SYSTEMTIME daylightTimeRule;
    };

    // Create default time zone
    QWinTimeZonePrivate();
    // Create named time zone
    QWinTimeZonePrivate(const QByteArray &ianaId);
    ~QWinTimeZonePrivate();

    QWinTimeZonePrivate *clone() const override;

    QString comment() const override;

    using QTimeZonePrivate::displayName;
    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const override;
    QString abbreviation(qint64 atMSecsSinceEpoch) const override;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const override;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const override;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const override;

    bool hasDaylightTime() const override;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const override;

    using QTimeZonePrivate::data;
    Data data(qint64 forMSecsSinceEpoch) const override;

    bool hasTransitions() const override;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const override;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const override;

    QByteArray systemTimeZoneId() const override;

    QList<QByteArray> availableTimeZoneIds() const override;

    // For use within implementation's TransitionTimePair:
    QTimeZonePrivate::Data ruleToData(const QWinTransitionRule &rule, qint64 atMSecsSinceEpoch,
                                      QTimeZone::TimeType type, bool fakeDst = false) const;
private:
    void init(const QByteArray &ianaId);

    QByteArray m_windowsId;
    QString m_displayName;
    QString m_standardName;
    QString m_daylightName;
    QList<QWinTransitionRule> m_tranRules;
};
#endif // C++20, Darwin, Android, Unix, ICU, Win.

QT_END_NAMESPACE

#endif // QTIMEZONEPRIVATE_P_H
