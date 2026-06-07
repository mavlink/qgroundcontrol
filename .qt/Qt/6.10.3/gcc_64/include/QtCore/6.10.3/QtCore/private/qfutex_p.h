// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUTEX_P_H
#define QFUTEX_P_H

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

#include <qdeadlinetimer.h>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtDummyFutex {
    constexpr inline bool futexAvailable() { return false; }
    template <typename Atomic>
    inline bool futexWait(Atomic &, typename Atomic::Type, QDeadlineTimer = {})
    { Q_UNREACHABLE_RETURN(false); }
    template <typename Atomic> inline void futexWakeOne(Atomic &)
    { Q_UNREACHABLE(); }
    template <typename Atomic> inline void futexWakeAll(Atomic &)
    { Q_UNREACHABLE(); }
}

QT_END_NAMESPACE

#if defined(Q_OS_DARWIN)
#  include "qfutex_mac_p.h"
#elif defined(Q_OS_FREEBSD)
#  include "qfutex_freebsd_p.h"
#elif defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
// use Linux mutexes everywhere except for LSB builds
#  include "qfutex_linux_p.h"
#elif defined(Q_OS_WIN)
#  include "qfutex_win_p.h"
#else
QT_BEGIN_NAMESPACE
namespace QtFutex = QtDummyFutex;
QT_END_NAMESPACE
#endif

#endif // QFUTEX_P_H
