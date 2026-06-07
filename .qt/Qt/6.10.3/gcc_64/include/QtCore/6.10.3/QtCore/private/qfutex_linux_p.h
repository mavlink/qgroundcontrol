// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QFUTEX_LINUX_P_H
#define QFUTEX_LINUX_P_H

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

#include <private/qcore_unix_p.h>
#include <qdeadlinetimer.h>
#include <qtsan_impl.h>

#include <asm/unistd.h>
#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

// RISC-V does not supply __NR_futex
#ifndef __NR_futex
#  define __NR_futex __NR_futex_time64
#endif

#define QT_ALWAYS_USE_FUTEX

QT_BEGIN_NAMESPACE

namespace QtLinuxFutex {
constexpr inline bool futexAvailable() { return true; }

inline long _q_futex(int *addr, int op, int val, quintptr val2 = 0,
                     int *addr2 = nullptr, int val3 = 0) noexcept
{
    QtTsan::futexRelease(addr, addr2);

    // we use __NR_futex because some libcs (like Android's bionic) don't
    // provide SYS_futex etc.
    long result = syscall(__NR_futex, addr, op | FUTEX_PRIVATE_FLAG, val, val2, addr2, val3);

    QtTsan::futexAcquire(addr, addr2);

    return result;
}
template <typename T> int *addr(T *ptr)
{
    int *int_addr = reinterpret_cast<int *>(ptr);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    if (sizeof(T) > sizeof(int))
        int_addr++; //We want a pointer to the least significant half
#endif
    return int_addr;
}

template <typename Atomic>
inline void futexWait(Atomic &futex, typename Atomic::Type expectedValue)
{
    _q_futex(addr(&futex), FUTEX_WAIT, qintptr(expectedValue));
}
template <typename Atomic>
inline bool futexWait(Atomic &futex, typename Atomic::Type expectedValue, QDeadlineTimer deadline)
{
    auto timeout = deadline.deadline<std::chrono::steady_clock>().time_since_epoch();
    struct timespec ts = durationToTimespec(timeout);
    long r = _q_futex(addr(&futex), FUTEX_WAIT_BITSET, qintptr(expectedValue), quintptr(&ts),
                      nullptr, FUTEX_BITSET_MATCH_ANY);
    return r == 0 || errno != ETIMEDOUT;
}
template <typename Atomic> inline void futexWakeOne(Atomic &futex)
{
    _q_futex(addr(&futex), FUTEX_WAKE, 1);
}
template <typename Atomic> inline void futexWakeAll(Atomic &futex)
{
    _q_futex(addr(&futex), FUTEX_WAKE, INT_MAX);
}
template <typename Atomic> inline
void futexWakeOp(Atomic &futex1, int wake1, int wake2, Atomic &futex2, quint32 op)
{
    _q_futex(addr(&futex1), FUTEX_WAKE_OP, wake1, wake2, addr(&futex2), op);
}} // namespace QtLinuxFutex
namespace QtFutex = QtLinuxFutex;

QT_END_NAMESPACE

#endif // QFUTEX_LINUX_P_H
