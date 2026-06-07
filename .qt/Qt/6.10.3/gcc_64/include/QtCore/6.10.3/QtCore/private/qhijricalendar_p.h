// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHIJRI_CALENDAR_P_H
#define QHIJRI_CALENDAR_P_H

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

#include <QtCore/private/qglobal_p.h>
#include "qcalendarbackend_p.h"

QT_REQUIRE_CONFIG(hijricalendar);

QT_BEGIN_NAMESPACE

// Base for sharing with other variants on the Islamic calendar, as needed:
class Q_CORE_EXPORT QHijriCalendar : public QCalendarBackend
{
public:
    int daysInMonth(int month, int year = QCalendar::Unspecified) const override;
    int maximumDaysInMonth() const override;
    int daysInYear(int year) const override;

    bool isLunar() const override;
    bool isLuniSolar() const override;
    bool isSolar() const override;

protected:
    const QCalendarLocale *localeMonthIndexData() const override;
    const char16_t *localeMonthData() const override;
};

QT_END_NAMESPACE

#endif // QHIJRI_CALENDAR_P_H
