#include "SignalHandler.h"

#include <QtCore/QCoreApplication>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SignalHandlerLog, "Utilities.SignalHandler")

std::atomic<SignalHandler*> SignalHandler::s_current{nullptr};

SignalHandler::SignalHandler(QObject* parent)
    : QObject(parent)
{
    s_current.store(this, std::memory_order_release);
    qCDebug(SignalHandlerLog) << this;
}

#ifdef Q_OS_WIN

#include <QtCore/QWinEventNotifier>
#include <qt_windows.h>

/// Plain C thunk with Windows signature. Delegates to class static.
static BOOL WINAPI _consoleCtrlHandler(DWORD evt)
{
    return SignalHandler::consoleCtrlHandler(static_cast<unsigned long>(evt)) ? TRUE : FALSE;
}

int SignalHandler::consoleCtrlHandler(unsigned long evt)
{
    switch (evt) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT: {
        SignalHandler* self = SignalHandler::current();
        if (!self || !self->_signalEvent || (self->_signalEvent == reinterpret_cast<Qt::HANDLE>(INVALID_HANDLE_VALUE))) {
            return 0;
        }
        (void) SetEvent(static_cast<HANDLE>(self->_signalEvent)); // non-blocking
        if ((evt != CTRL_C_EVENT) && (evt != CTRL_BREAK_EVENT)) {
            // Windows terminates the process when this handler returns, and after about 5 s in any case.
            // Stay here so that the requested quit can run: a normal exit ends this thread earlier.
            Sleep(5000);
        }
        return 1;
    }
    default:
        return 0;
    }
}

SignalHandler::~SignalHandler()
{
    if (_notifier) {
        _notifier->setEnabled(false);
    }

    if (_signalEvent && (_signalEvent != reinterpret_cast<Qt::HANDLE>(INVALID_HANDLE_VALUE))) {
        (void) CloseHandle(static_cast<HANDLE>(_signalEvent));
        _signalEvent = reinterpret_cast<Qt::HANDLE>(INVALID_HANDLE_VALUE);
    }

    (void) SetConsoleCtrlHandler(&_consoleCtrlHandler, FALSE);
    s_current.store(nullptr, std::memory_order_release);

    qCDebug(SignalHandlerLog) << this;
}

int SignalHandler::setupSignalHandlers()
{
    // Create auto-reset event. Initial state = non-signaled.
    _signalEvent = reinterpret_cast<Qt::HANDLE>(CreateEventW(/*lpEventAttributes*/ nullptr, /*bManualReset*/ FALSE, /*bInitialState*/ FALSE, /*lpName*/ nullptr));
    if (!_signalEvent || (_signalEvent == reinterpret_cast<Qt::HANDLE>(INVALID_HANDLE_VALUE))) {
        return 1;
    }

    _notifier = new QWinEventNotifier(static_cast<HANDLE>(_signalEvent), this);
    (void) connect(_notifier, &QWinEventNotifier::activated, this, [this]([[maybe_unused]] HANDLE handle) {
        // Auto-reset event already consumed. No drain needed.
        if (!std::exchange(_sigIntTriggered, true)) {
            qCDebug(SignalHandlerLog) << "Console event—press Ctrl+C again to exit immediately";
            QCoreApplication::quit();
        } else {
            qCDebug(SignalHandlerLog) << "Caught second SIGINT—exiting immediately";
            _exit(1);
        }
    });

    if (!SetConsoleCtrlHandler(&_consoleCtrlHandler, TRUE)) {
        return 2;
    }

    return 0;
}

#else // POSIX

#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <QtCore/QSocketNotifier>

// The signal handlers may only use lock-free atomics
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<SignalHandler*>::is_always_lock_free);

SignalHandler::~SignalHandler()
{
    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    (void) sigaction(SIGINT, &sa, nullptr);
    (void) sigaction(SIGTERM, &sa, nullptr);

    if (_notifierInt) {
        _notifierInt->setEnabled(false);
    }
    if (_notifierTerm) {
        _notifierTerm->setEnabled(false);
    }

    for (int* fd : {&_sigIntFd[0], &_sigIntFd[1], &_sigTermFd[0], &_sigTermFd[1]}) {
        if (*fd >= 0) {
            (void) ::close(*fd);
            *fd = -1;
        }
    }

    s_current.store(nullptr, std::memory_order_release);
    qCDebug(SignalHandlerLog) << this;
}

int SignalHandler::setupSignalHandlers()
{
    // Datagram socketpairs: read end = [0], write end = [1]
    if (::socketpair(AF_UNIX, SOCK_DGRAM, 0, _sigIntFd)) {
        qCCritical(SignalHandlerLog) << "Failed to create SIGINT socketpair:" << strerror(errno);
        return 1;
    }
    if (::socketpair(AF_UNIX, SOCK_DGRAM, 0, _sigTermFd)) {
        qCCritical(SignalHandlerLog) << "Failed to create SIGTERM socketpair:" << strerror(errno);
        return 2;
    }

    for (const int fd : {_sigIntFd[0], _sigIntFd[1], _sigTermFd[0], _sigTermFd[1]}) {
        // Close-on-exec to prevent fd leaks into children
        (void) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD, 0) | FD_CLOEXEC);
        // Non-blocking on both ends: a signal handler must not block when its socket is full
        (void) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    }

    _notifierInt  = new QSocketNotifier(_sigIntFd[0],  QSocketNotifier::Read, this);
    _notifierTerm = new QSocketNotifier(_sigTermFd[0], QSocketNotifier::Read, this);
    (void) connect(_notifierInt,  &QSocketNotifier::activated, this, &SignalHandler::_onSigInt);
    (void) connect(_notifierTerm, &QSocketNotifier::activated, this, &SignalHandler::_onSigTerm);

    struct sigaction sa{};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SignalHandler::_intSignalHandler;
    if (sigaction(SIGINT, &sa, nullptr)) {
        return 3;
    }
    sa.sa_handler = SignalHandler::_termSignalHandler;
    if (sigaction(SIGTERM, &sa, nullptr)) {
        return 4;
    }

    return 0;
}

void SignalHandler::_onSigInt()
{
    char b;
    [[maybe_unused]] const ssize_t n = ::read(_sigIntFd[0], &b, 1); // single-byte drain

    // A second SIGINT never gets here: _intSignalHandler() exits the process
    qCInfo(SignalHandlerLog) << "Caught SIGINT—press Ctrl+C again to exit immediately";
    QCoreApplication::quit();
}

void SignalHandler::_onSigTerm()
{
    char b;
    [[maybe_unused]] const ssize_t n = ::read(_sigTermFd[0], &b, 1); // single-byte drain

    qCDebug(SignalHandlerLog) << "Caught SIGTERM—shutting down gracefully";
    QCoreApplication::quit();
}

// The functions below run in signal context: async-signal-safe calls only (see signal-safety(7)), no logging.

void SignalHandler::_intSignalHandler(int signum)
{
    SignalHandler* self = current();
    if (!self) {
        return;
    }
    // Decided here and not in _onSigInt(), which never runs when the event loop is blocked
    if (self->_sigIntCount.fetch_add(1, std::memory_order_relaxed) > 0) {
        ::_exit(128 + signum);
    }
    _wakeEventLoop(self->_sigIntFd[1]);
}

void SignalHandler::_termSignalHandler([[maybe_unused]] int signum)
{
    const SignalHandler* self = current();
    if (!self) {
        return;
    }
    _wakeEventLoop(self->_sigTermFd[1]);
}

void SignalHandler::_wakeEventLoop(int fd)
{
    const int savedErrno = errno;
    const char b = 1;
    // Fails when the socket is full, in which case a wake-up is already pending
    [[maybe_unused]] const ssize_t n = ::write(fd, &b, sizeof(b));
    errno = savedErrno;
}

#endif // POSIX
