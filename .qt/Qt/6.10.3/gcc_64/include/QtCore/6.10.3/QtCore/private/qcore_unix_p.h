// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCORE_UNIX_P_H
#define QCORE_UNIX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt code on Unix. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qplatformdefs.h"
#include <QtCore/private/qglobal_p.h>
#include "qbytearray.h"
#include "qdeadlinetimer.h"

#ifndef Q_OS_UNIX
# error "qcore_unix_p.h included on a non-Unix system"
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if !defined (Q_OS_VXWORKS)
# if !defined(Q_OS_HPUX) || defined(__ia64)
#  include <sys/select.h>
# endif
#  include <sys/time.h>
#else
#  include <selectLib.h>
#endif

#include <chrono>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#if defined(Q_OS_VXWORKS)
#  include <ioLib.h>
#endif

#ifdef QT_NO_NATIVE_POLL
#  include "qpoll_p.h"
#else
#  include <poll.h>
#endif

struct sockaddr;

#define QT_EINTR_LOOP(var, cmd)                 \
    do {                                        \
        var = cmd;                              \
    } while (var == -1 && errno == EINTR)

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(pollfd, Q_PRIMITIVE_TYPE);

static inline constexpr clockid_t SteadyClockClockId =
#if !defined(CLOCK_MONOTONIC)
        // we don't know how to set the monotonic clock
        CLOCK_REALTIME
#elif defined(_LIBCPP_VERSION) && defined(_LIBCPP_HAS_NO_MONOTONIC_CLOCK)
        // libc++ falling back to system_clock
        CLOCK_REALTIME
#elif defined(__GLIBCXX__) && !defined(_GLIBCXX_USE_CLOCK_MONOTONIC)
        // libstdc++ falling back to system_clock
        CLOCK_REALTIME
#elif defined(_LIBCPP_VERSION) && defined(Q_OS_DARWIN)
        // on Apple systems, libc++ uses CLOCK_MONOTONIC_RAW since LLVM 11
        // https://github.com/llvm/llvm-project/blob/llvmorg-11.0.0/libcxx/src/chrono.cpp#L117-L129
        CLOCK_MONOTONIC_RAW
#elif defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
        // both libstdc++ and libc++ otherwise use CLOCK_MONOTONIC
        CLOCK_MONOTONIC
#else
#  warning "Unknown C++ Standard Library implementation - code may be sub-optimal"
        CLOCK_REALTIME
#endif
        ;

static inline constexpr clockid_t QWaitConditionClockId =
#if !QT_CONFIG(thread)
        // bootstrap mode, there are no wait conditions
        CLOCK_REALTIME
#elif !QT_CONFIG(pthread_condattr_setclock)
        // OSes that lack pthread_condattr_setclock() (e.g., Darwin)
        CLOCK_REALTIME
#elif defined(Q_OS_QNX)
        // unknown why use of the monotonic clock causes failures
        CLOCK_REALTIME
#else
        SteadyClockClockId;
#endif
        ;

static constexpr auto OneSecAsNsecs = std::chrono::nanoseconds(std::chrono::seconds{ 1 }).count();

inline timespec durationToTimespec(std::chrono::nanoseconds timeout) noexcept
{
    using namespace std::chrono;
    const seconds secs = duration_cast<seconds>(timeout);
    const nanoseconds frac = timeout - secs;
    struct timespec ts;
    ts.tv_sec = secs.count();
    ts.tv_nsec = frac.count();
    return ts;
}

template <typename Duration>
inline Duration timespecToChrono(timespec ts) noexcept
{
    using namespace std::chrono;
    return duration_cast<Duration>(seconds{ts.tv_sec} + nanoseconds{ts.tv_nsec});
}

inline std::chrono::milliseconds timespecToChronoMs(timespec ts) noexcept
{
    return timespecToChrono<std::chrono::milliseconds>(ts);
}

// Internal operator functions for timespecs
constexpr inline timespec &normalizedTimespec(timespec &t)
{
    while (t.tv_nsec >= OneSecAsNsecs) {
        ++t.tv_sec;
        t.tv_nsec -= OneSecAsNsecs;
    }
    while (t.tv_nsec < 0) {
        --t.tv_sec;
        t.tv_nsec += OneSecAsNsecs;
    }
    return t;
}
constexpr inline bool operator<(const timespec &t1, const timespec &t2)
{ return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec); }
constexpr inline bool operator==(const timespec &t1, const timespec &t2)
{ return t1.tv_sec == t2.tv_sec && t1.tv_nsec == t2.tv_nsec; }
constexpr inline bool operator!=(const timespec &t1, const timespec &t2)
{ return !(t1 == t2); }
constexpr inline timespec &operator+=(timespec &t1, const timespec &t2)
{
    t1.tv_sec += t2.tv_sec;
    t1.tv_nsec += t2.tv_nsec;
    return normalizedTimespec(t1);
}
constexpr inline timespec operator+(const timespec &t1, const timespec &t2)
{
    timespec tmp = {};
    tmp.tv_sec = t1.tv_sec + t2.tv_sec;
    tmp.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    return normalizedTimespec(tmp);
}
constexpr inline timespec operator-(const timespec &t1, const timespec &t2)
{
    timespec tmp = {};
    tmp.tv_sec = t1.tv_sec - (t2.tv_sec - 1);
    tmp.tv_nsec = t1.tv_nsec - (t2.tv_nsec + OneSecAsNsecs);
    return normalizedTimespec(tmp);
}
constexpr inline timespec operator*(const timespec &t1, int mul)
{
    timespec tmp = {};
    tmp.tv_sec = t1.tv_sec * mul;
    tmp.tv_nsec = t1.tv_nsec * mul;
    return normalizedTimespec(tmp);
}
inline timeval timespecToTimeval(timespec ts)
{
    timeval tv;
    tv.tv_sec = ts.tv_sec;
    tv.tv_usec = ts.tv_nsec / 1000;
    return tv;
}

inline timespec &operator+=(timespec &t1, std::chrono::milliseconds msecs)
{
    t1 += durationToTimespec(msecs);
    return t1;
}

inline timespec &operator+=(timespec &t1, int ms)
{
    t1 += std::chrono::milliseconds{ms};
    return t1;
}

inline timespec operator+(const timespec &t1, std::chrono::milliseconds msecs)
{
    timespec tmp = t1;
    tmp += msecs;
    return tmp;
}

inline timespec operator+(const timespec &t1, int ms)
{
    return t1 + std::chrono::milliseconds{ms};
}

inline timespec qAbsTimespec(timespec ts)
{
    if (ts.tv_sec < 0) {
        ts.tv_sec = -ts.tv_sec - 1;
        ts.tv_nsec -= OneSecAsNsecs;
    }
    if (ts.tv_sec == 0 && ts.tv_nsec < 0) {
        ts.tv_nsec = -ts.tv_nsec;
    }
    return normalizedTimespec(ts);
}

template <clockid_t ClockId = SteadyClockClockId>
inline timespec deadlineToAbstime(QDeadlineTimer deadline)
{
    using namespace std::chrono;
    using Clock =
        std::conditional_t<ClockId == CLOCK_REALTIME, system_clock, steady_clock>;
    auto timePoint = deadline.deadline<Clock>();
    if (timePoint < typename Clock::time_point{})
        return {};
    return durationToTimespec(timePoint.time_since_epoch());
}

Q_CORE_EXPORT void qt_ignore_sigpipe() noexcept;

#if defined(Q_PROCESSOR_X86_32) && defined(__GLIBC__)
#  if !__GLIBC_PREREQ(2, 22)
Q_CORE_EXPORT int qt_open64(const char *pathname, int flags, mode_t);
#    undef QT_OPEN
#    define QT_OPEN qt_open64
#  endif
#endif

#ifdef AT_FDCWD
static inline int qt_safe_openat(int dfd, const char *pathname, int flags, mode_t mode = 0777)
{
    // everyone already has O_CLOEXEC
    int fd;
    QT_EINTR_LOOP(fd, openat(dfd, pathname, flags | O_CLOEXEC, mode));
    return fd;
}
#endif

// don't call QT_OPEN or ::open
// call qt_safe_open
static inline int qt_safe_open(const char *pathname, int flags, mode_t mode = 0777)
{
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    int fd;
    QT_EINTR_LOOP(fd, QT_OPEN(pathname, flags, mode));

#ifndef O_CLOEXEC
    if (fd != -1)
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif

    return fd;
}
#undef QT_OPEN
#define QT_OPEN         qt_safe_open

// don't call ::pipe
// call qt_safe_pipe
static inline int qt_safe_pipe(int pipefd[2], int flags = 0)
{
    Q_ASSERT((flags & ~O_NONBLOCK) == 0);

#ifdef QT_THREADSAFE_CLOEXEC
    // use pipe2
    flags |= O_CLOEXEC;
    return ::pipe2(pipefd, flags); // pipe2 is documented not to return EINTR
#else
    int ret = ::pipe(pipefd);
    if (ret == -1)
        return -1;

    ::fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

    // set non-block too?
    if (flags & O_NONBLOCK) {
        ::fcntl(pipefd[0], F_SETFL, ::fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
        ::fcntl(pipefd[1], F_SETFL, ::fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
    }

    return 0;
#endif
}

// don't call dup or fcntl(F_DUPFD)
static inline int qt_safe_dup(int oldfd, int atleast = 0, int flags = FD_CLOEXEC)
{
    Q_ASSERT(flags == FD_CLOEXEC || flags == 0);

#ifdef F_DUPFD_CLOEXEC
    int cmd = F_DUPFD;
    if (flags & FD_CLOEXEC)
        cmd = F_DUPFD_CLOEXEC;
    return ::fcntl(oldfd, cmd, atleast);
#else
    // use F_DUPFD
    int ret = ::fcntl(oldfd, F_DUPFD, atleast);

    if (flags && ret != -1)
        ::fcntl(ret, F_SETFD, flags);
    return ret;
#endif
}

// don't call dup2
// call qt_safe_dup2
static inline int qt_safe_dup2(int oldfd, int newfd, int flags = FD_CLOEXEC)
{
    Q_ASSERT(flags == FD_CLOEXEC || flags == 0);

    int ret;
#if QT_CONFIG(dup3)
    // use dup3
    QT_EINTR_LOOP(ret, ::dup3(oldfd, newfd, flags ? O_CLOEXEC : 0));
    return ret;
#else
    QT_EINTR_LOOP(ret, ::dup2(oldfd, newfd));
    if (ret == -1)
        return -1;

    if (flags)
        ::fcntl(newfd, F_SETFD, flags);
    return 0;
#endif
}

static inline qint64 qt_safe_read(int fd, void *data, qint64 maxlen)
{
    qint64 ret = 0;
    QT_EINTR_LOOP(ret, QT_READ(fd, data, maxlen));
    return ret;
}
#undef QT_READ
#define QT_READ qt_safe_read

static inline qint64 qt_safe_write(int fd, const void *data, qint64 len)
{
    qint64 ret = 0;
    QT_EINTR_LOOP(ret, QT_WRITE(fd, data, len));
    return ret;
}
#undef QT_WRITE
#define QT_WRITE qt_safe_write

static inline qint64 qt_safe_write_nosignal(int fd, const void *data, qint64 len)
{
    qt_ignore_sigpipe();
    return qt_safe_write(fd, data, len);
}

static inline int qt_safe_close(int fd)
{
    int ret;
    QT_EINTR_LOOP(ret, QT_CLOSE(fd));
    return ret;
}
#undef QT_CLOSE
#define QT_CLOSE qt_safe_close

// - VxWorks & iOS/tvOS/watchOS don't have processes
#if QT_CONFIG(process)
static inline int qt_safe_execve(const char *filename, char *const argv[],
                                 char *const envp[])
{
    int ret;
    QT_EINTR_LOOP(ret, ::execve(filename, argv, envp));
    return ret;
}

static inline int qt_safe_execv(const char *path, char *const argv[])
{
    int ret;
    QT_EINTR_LOOP(ret, ::execv(path, argv));
    return ret;
}

static inline int qt_safe_execvp(const char *file, char *const argv[])
{
    int ret;
    QT_EINTR_LOOP(ret, ::execvp(file, argv));
    return ret;
}

static inline pid_t qt_safe_waitpid(pid_t pid, int *status, int options)
{
    int ret;
    QT_EINTR_LOOP(ret, ::waitpid(pid, status, options));
    return ret;
}
#endif // QT_CONFIG(process)

#if !defined(_POSIX_MONOTONIC_CLOCK)
#  define _POSIX_MONOTONIC_CLOCK -1
#endif

QByteArray qt_readlink(const char *path);

/* non-static */
inline bool qt_haveLinuxProcfs()
{
#ifdef Q_OS_LINUX
#  ifdef QT_LINUX_ALWAYS_HAVE_PROCFS
    return true;
#  else
    static const bool present = (access("/proc/version", F_OK) == 0);
    return present;
#  endif
#else
    return false;
#endif
}

Q_CORE_EXPORT int qt_safe_poll(struct pollfd *fds, nfds_t nfds, QDeadlineTimer deadline);

static inline struct pollfd qt_make_pollfd(int fd, short events)
{
    struct pollfd pfd = { fd, events, 0 };
    return pfd;
}

// according to X/OPEN we have to define semun ourselves
// we use prefix as on some systems sem.h will have it
struct semid_ds;
union qt_semun {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short *array;      /* array for GETALL, SETALL */
};

QT_END_NAMESPACE

#endif
