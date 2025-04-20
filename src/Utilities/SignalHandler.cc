/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SignalHandler.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QGlobalStatic>
#include <QtCore/QSocketNotifier>
#include <QtQuick/QQuickWindow>

#include <sys/signal.h>
#include <sys/socket.h>

QGC_LOGGING_CATEGORY(SignalHandlerLog, "qgc.utilities.signalhandler")

int SignalHandler::sigIntFd[2] = {0, 0};
int SignalHandler::sigTermFd[2] = {0, 0};

Q_GLOBAL_STATIC(SignalHandler, _signalHandlerInstance);

SignalHandler *SignalHandler::instance()
{
    return _signalHandlerInstance();
}

SignalHandler::SignalHandler(QObject *parent)
    : QObject(parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigIntFd)) {
       qCFatal(SignalHandlerLog) << "Couldn't create INT socketpair";
    }
    _notifierInt = new QSocketNotifier(sigIntFd[1], QSocketNotifier::Read, this);
    (void) connect(_notifierInt, &QSocketNotifier::activated, this, &SignalHandler::_onSigInt);

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigTermFd)) {
       qCFatal(SignalHandlerLog) << "Couldn't create TERM socketpair";
    }
    _notifierTerm = new QSocketNotifier(sigTermFd[1], QSocketNotifier::Read, this);
    (void) connect(_notifierTerm, &QSocketNotifier::activated, this, &SignalHandler::_onSigTerm);
}

void SignalHandler::_onSigInt()
{
    _notifierInt->setEnabled(false);
    char b;
    ::read(sigIntFd[1], &b, sizeof(b));
    qCDebug(SignalHandlerLog) << "Caught SIGINT—shutting down gracefully";

    if (qgcApp() && qgcApp()->mainRootWindow()) {
        (void) qgcApp()->mainRootWindow()->close();
        QEvent ev(QEvent::Quit);
        (void) qgcApp()->event(&ev);
    }

    _notifierInt->setEnabled(true);
}

void SignalHandler::_onSigTerm()
{
    _notifierTerm->setEnabled(false);
    char b;
    ::read(sigTermFd[1], &b, sizeof(b));

    qCDebug(SignalHandlerLog) << "Caught SIGTERM—shutting down gracefully";
    if (qgcApp() && qgcApp()->mainRootWindow()) {
        (void) qgcApp()->mainRootWindow()->close();
        QEvent ev(QEvent::Quit);
        (void) qgcApp()->event(&ev);
    }

    _notifierTerm->setEnabled(true);
}

void SignalHandler::intSignalHandler(int signum)
{
    Q_ASSERT(signum == SIGINT);

    char b = 1;
    (void) ::write(sigIntFd[0], &b, sizeof(b));
}

void SignalHandler::termSignalHandler(int signum)
{
    Q_ASSERT(signum == SIGTERM);

    char b = 1;
    (void) ::write(sigTermFd[0], &b, sizeof(b));
}

int SignalHandler::setupSignalHandlers()
{
    struct sigaction sa_int{};
    sa_int.sa_handler = SignalHandler::intSignalHandler;
    (void) sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sa_int.sa_flags |= SA_RESTART;
    if (sigaction(SIGINT, &sa_int, nullptr)) {
        return 1;
    }

    struct sigaction sa_term{};
    sa_term.sa_handler = SignalHandler::termSignalHandler;
    (void) sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sa_term.sa_flags |= SA_RESTART;
    if (sigaction(SIGTERM, &sa_term, nullptr)) {
        return 2;
    }

    return 0;
}
