// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QJALALI_CALENDAR_P_H
#define QJALALI_CALENDAR_P_H

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

#include "qcalendarbackend_p.h"

QT_REQUIRE_CONFIG(jalalicalendar);

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QJalaliCalendar : public QCalendarBackend
{
public:
    // Calendar properties:
    QString name() const override;
    static QStringList nameList();
    // Date queries:
    int daysInMonth(int month, int year = QCalendar::Unspecified) const override;
    bool isLeapYear(int year) const override;
    // Properties of the calendar
    bool isLunar() const override;
    bool isLuniSolar() const override;
    bool isSolar() const override;
    // Julian Day conversions:
    bool dateToJulianDay(int year, int month, int day, qint64 *jd) const override;
    QCalendar::YearMonthDay julianDayToDate(qint64 jd) const override;

protected:
    // locale support:
    const QCalendarLocale *localeMonthIndexData() const override;
    const char16_t *localeMonthData() const override;
};

QT_END_NAMESPACE

#endif // QJALALI_CALENDAR_P_H
