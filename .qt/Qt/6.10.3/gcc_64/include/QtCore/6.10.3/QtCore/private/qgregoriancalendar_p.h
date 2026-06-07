// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGREGORIAN_CALENDAR_P_H
#define QGREGORIAN_CALENDAR_P_H

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

#include "qromancalendar_p.h"

#include <optional>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QGregorianCalendar : public QRomanCalendar
{
    // TODO: provide static methods, called by the overrides, that can be called
    // directly by QDate to optimize various parts.
public:
    // Calendar property:
    QString name() const override;
    static QStringList nameList();
    // Date queries:
    bool isLeapYear(int year) const override;
    // Julian Day conversions:
    bool dateToJulianDay(int year, int month, int day, qint64 *jd) const override;
    QCalendar::YearMonthDay julianDayToDate(qint64 jd) const override;
    qint64 matchCenturyToWeekday(const QCalendar::YearMonthDay &parts, int dow) const override;

    // Static optimized versions for the benefit of QDate:
    static int weekDayOfJulian(qint64 jd);
    static bool leapTest(int year);
    static int monthLength(int month, int year);
    static bool validParts(int year, int month, int day);
    static QCalendar::YearMonthDay partsFromJulian(qint64 jd);
    static std::optional<qint64> julianFromParts(int year, int month, int day);
    // Used internally:
    static int yearStartWeekDay(int year);
    static int yearSharingWeekDays(QDate date);
};

QT_END_NAMESPACE

#endif // QGREGORIAN_CALENDAR_P_H
