// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGETTID_P_H
#define QGETTID_P_H

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

#include "qglobal_p.h"

#if defined(Q_OS_LINUX) && (defined(__GLIBC__) || __has_include(<sys/syscall.h>))
#  include <sys/syscall.h>
#  include <unistd.h>

# if defined(Q_OS_ANDROID) && !defined(SYS_gettid)
#  define SYS_gettid __NR_gettid
# endif

static inline long qt_gettid()
{
    // no error handling
    // this syscall has existed since Linux 2.4.11 and cannot fail
    return syscall(SYS_gettid);
}
#elif defined(Q_OS_DARWIN)
#  include <pthread.h>
static inline int qt_gettid()
{
    // no error handling: this call cannot fail
    __uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return tid;
}
#elif defined(Q_OS_FREEBSD_KERNEL) && defined(__FreeBSD_version) && __FreeBSD_version >= 900031
#  include <pthread_np.h>
static inline int qt_gettid()
{
    return pthread_getthreadid_np();
}
#else
#  include <qthread.h>
static QT_PREPEND_NAMESPACE(qint64) qt_gettid()
{
    QT_USE_NAMESPACE
    return qintptr(QThread::currentThreadId());
}
#endif

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

#endif // QGETTID_P_H
