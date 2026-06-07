// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCALENDAR_BACKEND_P_H
#define QCALENDAR_BACKEND_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of calendar implementations.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobjectdefs.h>
#include <QtCore/qcalendar.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qstring.h>
#include <QtCore/qmap.h>
#include <QtCore/qanystringview.h>
#include <QtCore/private/qlocale_p.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
class QCalendarRegistry;
}

// Locale-related parts, mostly handled in ../text/qlocale.cpp

struct QCalendarLocale {
    quint16 m_language_id, m_script_id, m_territory_id;

#define CASE(E, member) case QLocale::FormatType::E: \
        return { m_ ## member ## _idx, m_ ## member ## _size }
    QLocaleData::DataRange monthName(QLocale::FormatType type) const
    {
        switch (type) {
        CASE(LongFormat, longMonth);
        CASE(ShortFormat, shortMonth);
        CASE(NarrowFormat, narrowMonth);
        }
        Q_UNREACHABLE_RETURN({});
    }
    QLocaleData::DataRange standaloneMonthName(QLocale::FormatType type) const
    {
        switch (type) {
        CASE(LongFormat, longMonthStandalone);
        CASE(ShortFormat, shortMonthStandalone);
        CASE(NarrowFormat, narrowMonthStandalone);
        }
        Q_UNREACHABLE_RETURN({});
    }
#undef CASE

    // Month name indexes:
    quint16 m_longMonthStandalone_idx, m_longMonth_idx;
    quint16 m_shortMonthStandalone_idx, m_shortMonth_idx;
    quint16 m_narrowMonthStandalone_idx, m_narrowMonth_idx;

    // Twelve long month names (separated by commas) can add up to more than 256
    // QChars - e.g. kde_TZ gets to 264.
    quint16 m_longMonthStandalone_size, m_longMonth_size;
    quint8 m_shortMonthStandalone_size, m_shortMonth_size;
    quint8 m_narrowMonthStandalone_size, m_narrowMonth_size;
};

// Partial implementation, of methods with common forms, in qcalendar.cpp
class Q_CORE_EXPORT QCalendarBackend
{
    friend class QCalendar;
    friend class QtPrivate::QCalendarRegistry;
    Q_DISABLE_COPY_MOVE(QCalendarBackend)

public:
    QCalendarBackend() = default;
    virtual ~QCalendarBackend();
    virtual QString name() const = 0;

    QStringList names() const;

    QCalendar::System calendarSystem() const;
    QCalendar::SystemId calendarId() const { return m_id; }
    // Date queries:
    virtual int daysInMonth(int month, int year = QCalendar::Unspecified) const = 0;
    virtual int daysInYear(int year) const;
    virtual int monthsInYear(int year) const;
    virtual bool isDateValid(int year, int month, int day) const;
    // Properties of the calendar:
    virtual bool isLeapYear(int year) const = 0;
    virtual bool isLunar() const = 0;
    virtual bool isLuniSolar() const = 0;
    virtual bool isSolar() const = 0;
    virtual bool isProleptic() const;
    virtual bool hasYearZero() const;
    virtual int maximumDaysInMonth() const;
    virtual int minimumDaysInMonth() const;
    virtual int maximumMonthsInYear() const;
    // Julian Day conversions:
    virtual bool dateToJulianDay(int year, int month, int day, qint64 *jd) const = 0;
    virtual QCalendar::YearMonthDay julianDayToDate(qint64 jd) const = 0;
    // Day of week:
    virtual int dayOfWeek(qint64 jd) const;
    virtual qint64 matchCenturyToWeekday(const QCalendar::YearMonthDay &parts, int dow) const;

    // Names of months and week-days (implemented in qlocale.cpp):
    virtual QString monthName(const QLocale &locale, int month, int year,
                              QLocale::FormatType format) const;
    virtual QString standaloneMonthName(const QLocale &locale, int month, int year,
                                        QLocale::FormatType format) const;
    virtual QString weekDayName(const QLocale &locale, int day,
                                QLocale::FormatType format) const;
    virtual QString standaloneWeekDayName(const QLocale &locale, int day,
                                          QLocale::FormatType format) const;

    // Formatting of date-times (implemented in qlocale.cpp):
    virtual QString dateTimeToString(QStringView format, const QDateTime &datetime,
                                     QDate dateOnly, QTime timeOnly,
                                     const QLocale &locale) const;

    bool isGregorian() const;

    QCalendar::SystemId registerCustomBackend(const QStringList &names);

    // Calendar enumeration by name:
    static QStringList availableCalendars();

protected:
    // Locale support:
    virtual const QCalendarLocale *localeMonthIndexData() const = 0;
    virtual const char16_t *localeMonthData() const = 0;

private:
    QCalendar::SystemId m_id;

    void setIndex(size_t index);

    // QCalendar's access to its registry:
    static const QCalendarBackend *fromName(QAnyStringView name);
    static const QCalendarBackend *fromId(QCalendar::SystemId id);
    // QCalendar's access to singletons:
    static const QCalendarBackend *fromEnum(QCalendar::System system);
    static const QCalendarBackend *gregorian();
};

QT_END_NAMESPACE

#endif // QCALENDAR_BACKEND_P_H
