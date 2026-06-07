// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2015 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTENVIRONMENTVARIABLES_P_H
#define QTENVIRONMENTVARIABLES_P_H
// Nothing but (tests and) ../time/qlocaltime.cpp should access this.
#if defined(__cplusplus)

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an implementation
// detail. This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#include "qglobal_p.h"

#ifdef Q_CC_MINGW
#  include <unistd.h> // Define _POSIX_THREAD_SAFE_FUNCTIONS to obtain localtime_r()
#endif
#include <time.h>

QT_BEGIN_NAMESPACE

// These (behave as if they) consult the environment, so need to share its locking:
Q_CORE_EXPORT void qTzSet();
Q_CORE_EXPORT time_t qMkTime(struct tm *when);
Q_CORE_EXPORT bool qLocalTime(time_t utc, struct tm *local);
Q_CORE_EXPORT QString qTzName(int dstIndex);

QT_END_NAMESPACE

#endif // defined(__cplusplus)
#endif // QTENVIRONMENTVARIABLES_P_H
