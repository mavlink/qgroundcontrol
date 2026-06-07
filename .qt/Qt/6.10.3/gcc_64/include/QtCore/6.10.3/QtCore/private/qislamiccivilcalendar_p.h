// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QISLAMIC_CIVIL_CALENDAR_P_H
#define QISLAMIC_CIVIL_CALENDAR_P_H

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

#include "qhijricalendar_p.h"

QT_REQUIRE_CONFIG(islamiccivilcalendar);

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QIslamicCivilCalendar : public QHijriCalendar
{
public:
    // Calendar properties:
    QString name() const override;
    static QStringList nameList();
    // Date queries:
    bool isLeapYear(int year) const override;
    // Julian Day conversions:
    bool dateToJulianDay(int year, int month, int day, qint64 *jd) const override;
    QCalendar::YearMonthDay julianDayToDate(qint64 jd) const override;
};

QT_END_NAMESPACE

#endif // QISLAMIC_CIVIL_CALENDAR_P_H
