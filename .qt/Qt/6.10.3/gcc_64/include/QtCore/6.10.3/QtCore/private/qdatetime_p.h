// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDATETIME_P_H
#define QDATETIME_P_H

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
#include "QtCore/qdatetime.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qtimezone.h"

#if QT_CONFIG(timezone)
#include "qtimezone.h"
#endif

#include <chrono>

QT_BEGIN_NAMESPACE

class QDateTimePrivate : public QSharedData
{
public:
    // forward the declarations from QDateTime (this makes them public)
    typedef QDateTime::ShortData QDateTimeShortData;
    typedef QDateTime::Data QDateTimeData;

    // Never change or delete this enum, it is required for backwards compatible
    // serialization of QDateTime before 5.2, so is essentially public API
    enum Spec {
        LocalUnknown = -1,
        LocalStandard = 0,
        LocalDST = 1,
        UTC = 2,
        OffsetFromUTC = 3,
        TimeZone = 4
    };

    // Daylight Time Status
    enum DaylightStatus {
        UnknownDaylightTime = -1,
        StandardTime = 0,
        DaylightTime = 1
    };

    // Status of date/time
    enum StatusFlag {
        ShortData           = 0x01,

        ValidDate           = 0x02,
        ValidTime           = 0x04,
        ValidDateTime       = 0x08,

        TimeSpecMask        = 0x30,

        SetToStandardTime   = 0x40,
        SetToDaylightTime   = 0x80,
        ValidityMask        = ValidDate | ValidTime | ValidDateTime,
        DaylightMask        = SetToStandardTime | SetToDaylightTime,
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)


    enum TransitionOption {
        // Handling of a spring-forward (or other gap):
        GapUseBefore = 2,
        GapUseAfter = 4,
        // Handling of a fall-back (or other repeated period):
        FoldUseBefore = 0x20,
        FoldUseAfter = 0x40,
        // Quirk for negative DST:
        FlipForReverseDst = 0x400,

        GapMask = GapUseBefore | GapUseAfter,
        FoldMask = FoldUseBefore | FoldUseAfter,
    };
    Q_DECLARE_FLAGS(TransitionOptions, TransitionOption)

    enum {
        TimeSpecShift = 4,
    };

    struct ZoneState {
        qint64 when; // ms after zone/local 1970 start; may be revised from the input time.
        int offset = 0; // seconds
        DaylightStatus dst = UnknownDaylightTime;
        // Other fields are set, if possible, even when valid is false due to spring-forward.
        bool valid = false;

        ZoneState(qint64 local) : when(local) {}
        ZoneState(qint64 w, int o, DaylightStatus d, bool v = true)
            : when(w), offset(o), dst(d), valid(v) {}
    };

    static QDateTime::Data create(QDate toDate, QTime toTime, const QTimeZone &timeZone,
                                  QDateTime::TransitionResolution resolve);
#if QT_CONFIG(timezone)
    static ZoneState zoneStateAtMillis(const QTimeZone &zone, qint64 millis,
                                       TransitionOptions resolve);
#endif // timezone

    static ZoneState expressUtcAsLocal(qint64 utcMSecs);

    static ZoneState localStateAtMillis(qint64 millis, TransitionOptions resolve);
    static QString localNameAtMillis(qint64 millis, DaylightStatus dst); // empty if unknown

    StatusFlags m_status = StatusFlag(Qt::LocalTime << TimeSpecShift);
    qint64 m_msecs = 0;
    int m_offsetFromUtc = 0;
    QTimeZone m_timeZone;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimePrivate::StatusFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QDateTimePrivate::TransitionOptions)

namespace QtPrivate {
namespace DateTimeConstants {
using namespace std::chrono;
inline
constexpr qint64 SECS_PER_MIN = minutes::period::num;
inline
constexpr qint64 SECS_PER_HOUR = hours::period::num;
inline
constexpr qint64 SECS_PER_DAY = SECS_PER_HOUR * 24; // std::chrono::days is C++20

inline
constexpr qint64 MINS_PER_HOUR = std::ratio_divide<hours::period, minutes::period>::num;

inline
constexpr qint64 MSECS_PER_SEC = milliseconds::period::den;
inline
constexpr qint64 MSECS_PER_MIN = SECS_PER_MIN * MSECS_PER_SEC;
inline
constexpr qint64 MSECS_PER_HOUR = SECS_PER_HOUR * MSECS_PER_SEC;
inline
constexpr qint64 MSECS_PER_DAY = SECS_PER_DAY * MSECS_PER_SEC;

inline
constexpr qint64 JULIAN_DAY_FOR_EPOCH = 2440588; // result of QDate(1970, 1, 1).toJulianDay()

inline
constexpr qint64 JulianDayMax = Q_INT64_C( 784354017364);
inline
constexpr qint64 JulianDayMin = Q_INT64_C(-784350574879);
}
}

QT_END_NAMESPACE

#endif // QDATETIME_P_H
