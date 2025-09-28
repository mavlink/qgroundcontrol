/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SignalHandler.h"

#include <QtCore/QCoreApplication>
#ifdef Q_OS_WIN
#include <QtCore/QWinEventNotifier>
#else
#include <QtCore/QSocketNotifier>
#endif

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
            _exit(0);
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

    if (_sigIntFd[0] >= 0) {
        (void) ::close(_sigIntFd[0]);
        _sigIntFd[0] = -1;
    }
    if (_sigIntFd[1] >= 0) {
        (void) ::close(_sigIntFd[1]);
        _sigIntFd[1] = -1;
    }
    if (_sigTermFd[0] >= 0) {
        (void) ::close(_sigTermFd[0]);
        _sigTermFd[0] = -1;
    }
    if (_sigTermFd[1] >= 0) {
        (void) ::close(_sigTermFd[1]);
        _sigTermFd[1] = -1;
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

    // Close-on-exec to prevent fd leaks into children
    (void) fcntl(_sigIntFd[0],  F_SETFD, fcntl(_sigIntFd[0],  F_GETFD, 0) | FD_CLOEXEC);
    (void) fcntl(_sigIntFd[1],  F_SETFD, fcntl(_sigIntFd[1],  F_GETFD, 0) | FD_CLOEXEC);
    (void) fcntl(_sigTermFd[0], F_SETFD, fcntl(_sigTermFd[0], F_GETFD, 0) | FD_CLOEXEC);
    (void) fcntl(_sigTermFd[1], F_SETFD, fcntl(_sigTermFd[1], F_GETFD, 0) | FD_CLOEXEC);

    // Non-blocking read ends
    (void) fcntl(_sigIntFd[0],  F_SETFL, fcntl(_sigIntFd[0],  F_GETFL, 0) | O_NONBLOCK);
    (void) fcntl(_sigTermFd[0], F_SETFL, fcntl(_sigTermFd[0], F_GETFL, 0) | O_NONBLOCK);

    _notifierInt  = new QSocketNotifier(_sigIntFd[0],  QSocketNotifier::Read, this);
    _notifierTerm = new QSocketNotifier(_sigTermFd[0], QSocketNotifier::Read, this);
    (void) connect(_notifierInt,  &QSocketNotifier::activated, this, &SignalHandler::_onSigInt);
    (void) connect(_notifierTerm, &QSocketNotifier::activated, this, &SignalHandler::_onSigTerm);

    struct sigaction sa_int{};
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sa_int.sa_handler = SignalHandler::_intSignalHandler;
    if (sigaction(SIGINT, &sa_int, nullptr)) {
        return 3;
    }

    struct sigaction sa_term{};
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sa_term.sa_handler = SignalHandler::_termSignalHandler;
    if (sigaction(SIGTERM, &sa_term, nullptr)) {
        return 4;
    }

    return 0;
}

void SignalHandler::_onSigInt()
{
    char b;
    [[maybe_unused]] const ssize_t n = ::read(_sigIntFd[0], &b, 1); // single-byte drain

    if (!std::exchange(_sigIntTriggered, true)) {
        qCInfo(SignalHandlerLog) << "Caught SIGINT—press Ctrl+C again to exit immediately";
        QCoreApplication::quit();
    } else {
        qCDebug(SignalHandlerLog) << "Caught second SIGINT—exiting immediately";
        _exit(0);
    }
}

void SignalHandler::_onSigTerm()
{
    char b;
    [[maybe_unused]] const ssize_t n = ::read(_sigTermFd[0], &b, 1); // single-byte drain

    qCDebug(SignalHandlerLog) << "Caught SIGTERM—shutting down gracefully";
    QCoreApplication::quit();
}

void SignalHandler::_intSignalHandler(int signum)
{
    if (signum != SIGINT) {
        return;
    }
    const SignalHandler* self = current();
    if (!self) {
        return;
    }
    const char b = 1;
    [[maybe_unused]] const ssize_t n = ::write(self->_sigIntFd[1], &b, sizeof(b)); // no logging from signal handler
}

void SignalHandler::_termSignalHandler(int signum)
{
    if (signum != SIGTERM) {
        return;
    }
    const SignalHandler* self = current();
    if (!self) {
        return;
    }
    const char b = 1;
    [[maybe_unused]] const ssize_t n = ::write(self->_sigTermFd[1], &b, sizeof(b)); // no logging from signal handler
}

#endif // POSIX
