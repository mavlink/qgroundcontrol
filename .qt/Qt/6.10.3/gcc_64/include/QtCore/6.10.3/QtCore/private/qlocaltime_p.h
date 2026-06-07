// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOCALTIME_P_H
#define QLOCALTIME_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an implementation
// detail.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/private/qdatetime_p.h>

QT_BEGIN_NAMESPACE

// Packaging system time_t functions
namespace QLocalTime {
#ifndef QT_BOOTSTRAPPED
// Support for V4's Date implelenentation.
// Each returns offset from UTC in seconds (or 0 if unknown).
// V4 shall need to multiply by 1000.
// Offset is -ve East of Greenwich, +ve west of Greenwich.
// Add it to UTC seconds since epoch to get local seconds since nominal local epoch.
Q_CORE_EXPORT int getCurrentStandardUtcOffset();
Q_CORE_EXPORT int getUtcOffset(qint64 atMSecsSinceEpoch);
#endif // QT_BOOTSTRAPPED

// Support for QDateTime
QDateTimePrivate::ZoneState utcToLocal(qint64 utcMillis);
QString localTimeAbbbreviationAt(qint64 local, QDateTimePrivate::TransitionOptions resolve);
QDateTimePrivate::ZoneState mapLocalTime(qint64 local, QDateTimePrivate::TransitionOptions resolve);

struct SystemMillisRange { qint64 min, max; bool minClip, maxClip; };
SystemMillisRange computeSystemMillisRange();
}

QT_END_NAMESPACE

#endif // QLOCALTIME_P_H
