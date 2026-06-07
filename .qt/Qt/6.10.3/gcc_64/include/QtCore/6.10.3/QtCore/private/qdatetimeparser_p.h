// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDATETIMEPARSER_P_H
#define QDATETIMEPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "qplatformdefs.h"
#include "QtCore/qatomic.h"
#include "QtCore/qcalendar.h"
#include "QtCore/qcoreapplication.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qlist.h"
#include "QtCore/qlocale.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qtimezone.h"
#ifndef QT_BOOTSTRAPPED
# include "QtCore/qvariant.h"
#endif

QT_REQUIRE_CONFIG(datetimeparser);

#define QDATETIMEEDIT_TIME_MIN QTime(0, 0) // Prefer QDate::startOfDay()
#define QDATETIMEEDIT_TIME_MAX QTime(23, 59, 59, 999) // Prefer QDate::endOfDay()
#define QDATETIMEEDIT_DATE_MIN QDate(100, 1, 1)
#define QDATETIMEEDIT_COMPAT_DATE_MIN QDate(1752, 9, 14)
#define QDATETIMEEDIT_DATE_MAX QDate(9999, 12, 31)
#define QDATETIMEEDIT_DATE_INITIAL QDate(2000, 1, 1)

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QDateTimeParser
{
public:
    enum Context {
        FromString,
        DateTimeEdit
    };
    QDateTimeParser(QMetaType::Type t, Context ctx, QCalendar cal = QCalendar())
        : defaultLocale(QLocale::system()), parserType(t), context(ctx), calendar(cal)
    {
    }
    virtual ~QDateTimeParser();

    enum Section {
        NoSection     = 0x00000,
        AmPmSection   = 0x00001,
        MSecSection   = 0x00002,
        SecondSection = 0x00004,
        MinuteSection = 0x00008,
        Hour12Section   = 0x00010,
        Hour24Section   = 0x00020,
        TimeZoneSection = 0x00040,
        HourSectionMask = (Hour12Section | Hour24Section),
        TimeSectionMask = (MSecSection | SecondSection | MinuteSection |
                           HourSectionMask | AmPmSection | TimeZoneSection),

        DaySection         = 0x00100,
        MonthSection       = 0x00200,
        YearSection        = 0x00400,
        YearSection2Digits = 0x00800,
        YearSectionMask = YearSection | YearSection2Digits,
        DayOfWeekSectionShort = 0x01000,
        DayOfWeekSectionLong  = 0x02000,
        DayOfWeekSectionMask = DayOfWeekSectionShort | DayOfWeekSectionLong,
        DaySectionMask = DaySection | DayOfWeekSectionMask,
        DateSectionMask = DaySectionMask | MonthSection | YearSectionMask,

        Internal             = 0x10000,
        FirstSection         = 0x20000 | Internal,
        LastSection          = 0x40000 | Internal,
        CalendarPopupSection = 0x80000 | Internal,
    }; // extending qdatetimeedit.h's equivalent
    Q_DECLARE_FLAGS(Sections, Section)

    static constexpr int NoSectionIndex = -1;
    static constexpr int FirstSectionIndex = -2;
    static constexpr int LastSectionIndex = -3;

    struct Q_CORE_EXPORT SectionNode
    {
        constexpr SectionNode(Section tp, int ps, int ct, int zs = 0)
            : type(tp), pos(ps), count(ct), zeroesAdded(zs) {}
        Section type;
        mutable int pos;
        int count; // (used as Case(count) indicator for AmPmSection)
        int zeroesAdded;

        static QString name(Section s);
        QString name() const { return name(type); }
        QString format() const;
        int maxChange() const;
    };

    enum State { // duplicated from QValidator
        Invalid,
        Intermediate,
        Acceptable
    };

    struct StateNode {
        StateNode() : state(Invalid), padded(0), conflicts(false) {}
        StateNode(const QDateTime &val, State ok=Acceptable, int pad=0, bool bad=false)
            : value(val), state(ok), padded(pad), conflicts(bad) {}
        QDateTime value;
        State state;
        int padded;
        bool conflicts;
    };

    enum AmPm {
        AmText,
        PmText
    };

    StateNode parse(const QString &input, int position,
                    const QDateTime &defaultValue, bool fixup) const;
    bool fromString(const QString &text, QDate *date, QTime *time,
                    int baseYear = QLocale::DefaultTwoDigitBaseYear) const;
    bool fromString(const QString &text, QDateTime *datetime, int baseYear) const;
    bool parseFormat(QStringView format);

    enum FieldInfoFlag {
        Numeric = 0x01,
        FixedWidth = 0x02,
        AllowPartial = 0x04,
        Fraction = 0x08
    };
    Q_DECLARE_FLAGS(FieldInfo, FieldInfoFlag)

    FieldInfo fieldInfo(int index) const;

    void setDefaultLocale(const QLocale &loc) { defaultLocale = loc; }
    virtual QString displayText() const { return m_text; }
    void setCalendar(QCalendar calendar);

private:
    int sectionMaxSize(Section s, int count) const;
    QString sectionText(const QString &text, int sectionIndex, int index) const;
    StateNode scanString(const QDateTime &defaultValue, bool fixup) const;
    struct ParsedSection {
        int value;
        int used;
        int zeroes;
        State state;
        constexpr ParsedSection(State ok = Invalid,
                                       int val = 0, int read = 0, int zs = 0)
            : value(ok == Invalid ? -1 : val), used(read), zeroes(zs), state(ok)
            {}
    };
    ParsedSection parseSection(const QDateTime &currentValue, int sectionIndex, int offset) const;
    int findMonth(QStringView str, int monthstart, int sectionIndex,
                  int year, QString *monthName = nullptr, int *used = nullptr) const;
    int findDay(QStringView str, int intDaystart, int sectionIndex,
                QString *dayName = nullptr, int *used = nullptr) const;
    ParsedSection findUtcOffset(QStringView str, int mode) const;
    ParsedSection findTimeZoneName(QStringView str, const QDateTime &when) const;
    ParsedSection findTimeZone(QStringView str, const QDateTime &when,
                               int maxVal, int minVal, int mode) const;

    enum AmPmFinder {
        Neither = -1,
        AM = 0,
        PM = 1,
        PossibleAM = 2,
        PossiblePM = 3,
        PossibleBoth = 4
    };
    AmPmFinder findAmPm(QString &str, int index, int *used = nullptr) const;

    bool potentialValue(QStringView str, int min, int max, int index,
                        const QDateTime &currentValue, int insert) const;
    bool potentialValue(const QString &str, int min, int max, int index,
                        const QDateTime &currentValue, int insert) const
    {
        return potentialValue(QStringView(str), min, max, index, currentValue, insert);
    }

    enum Case {
        NativeCase,
        LowerCase,
        UpperCase
    };

    QString getAmPmText(AmPm ap, Case cs) const;
    QDateTime baseDate(const QTimeZone &zone) const;

    friend class QDTPUnitTestParser;

protected: // for the benefit of QDateTimeEditPrivate
    int sectionSize(int index) const;
    int sectionMaxSize(int index) const;
    int sectionPos(int index) const;
    int sectionPos(SectionNode sn) const;

    const SectionNode &sectionNode(int index) const;
    Section sectionType(int index) const;
    QString sectionText(int sectionIndex) const;
    int getDigit(const QDateTime &dt, int index) const;
    bool setDigit(QDateTime &t, int index, int newval) const;

    int absoluteMax(int index, const QDateTime &value = QDateTime()) const;
    int absoluteMin(int index) const;

    bool skipToNextSection(int section, const QDateTime &current, QStringView sectionText) const;
    bool skipToNextSection(int section, const QDateTime &current, const QString &sectionText) const
    {
        return skipToNextSection(section, current, QStringView(sectionText));
    }
    QString stateName(State s) const;

    virtual QDateTime getMinimum(const QTimeZone &zone) const;
    virtual QDateTime getMaximum(const QTimeZone &zone) const;
    virtual int cursorPosition() const { return -1; }
    virtual QLocale locale() const { return defaultLocale; }

    mutable int currentSectionIndex = int(NoSectionIndex);
    mutable int defaultCenturyStart = QLocale::DefaultTwoDigitBaseYear;
    Sections display;
    /*
        This stores the most recently selected day.
        It is useful when considering the following scenario:

        1. Date is: 31/01/2000
        2. User increments month: 29/02/2000
        3. User increments month: 31/03/2000

        At step 1, cachedDay stores 31. At step 2, the 31 is invalid for February, so the cachedDay is not updated.
        At step 3, the month is changed to March, for which 31 is a valid day. Since 29 < 31, the day is set to cachedDay.
        This is good for when users have selected their desired day and are scrolling up or down in the month or year section
        and do not want smaller months (or non-leap years) to alter the day that they chose.
    */
    mutable int cachedDay = -1;
    mutable QString m_text;
    QList<SectionNode> sectionNodes;
    QStringList separators;
    QString displayFormat;
    QLocale defaultLocale;
    QMetaType::Type parserType;
    bool fixday = false;
    Context context;
    QCalendar calendar;
};
Q_DECLARE_TYPEINFO(QDateTimeParser::SectionNode, Q_PRIMITIVE_TYPE);

Q_CORE_EXPORT bool operator==(QDateTimeParser::SectionNode s1, QDateTimeParser::SectionNode s2);

Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimeParser::Sections)
Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimeParser::FieldInfo)

QT_END_NAMESPACE

#endif // QDATETIME_P_H
