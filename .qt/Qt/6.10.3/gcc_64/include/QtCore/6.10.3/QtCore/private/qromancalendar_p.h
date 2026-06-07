// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QROMAN_CALENDAR_P_H
#define QROMAN_CALENDAR_P_H

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

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRomanCalendar : public QCalendarBackend
{
public:
    // date queries:
    int daysInMonth(int month, int year = QCalendar::Unspecified) const override;
    int minimumDaysInMonth() const override;
    // properties of the calendar
    bool isLunar() const override;
    bool isLuniSolar() const override;
    bool isSolar() const override;

    // Names of months (implemented in qlocale.cpp):
    QString monthName(const QLocale &locale, int month, int year,
                      QLocale::FormatType format) const override;
    QString standaloneMonthName(const QLocale &locale, int month, int year,
                                QLocale::FormatType format) const override;
protected:
    // locale support:
    const QCalendarLocale *localeMonthIndexData() const override;
    const char16_t *localeMonthData() const override;
};

QT_END_NAMESPACE

#endif // QROMAN_CALENDAR_P_H
