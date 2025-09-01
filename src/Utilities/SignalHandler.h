/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <atomic>

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#ifdef Q_OS_WIN
    class QWinEventNotifier;
#else
    class QSocketNotifier;
#endif

Q_DECLARE_LOGGING_CATEGORY(SignalHandlerLog)

class SignalHandler final : public QObject
{
    Q_OBJECT

public:
    explicit SignalHandler(QObject* parent = nullptr);
    ~SignalHandler() override;

    int setupSignalHandlers();

    static SignalHandler* current() { return s_current.load(std::memory_order_acquire); }

#ifdef Q_OS_WIN
    static int consoleCtrlHandler(unsigned long evt);
#else
private slots:
    void _onSigInt();
    void _onSigTerm();
#endif

private:
#ifdef Q_OS_WIN
    QWinEventNotifier* _notifier{nullptr};
    Qt::HANDLE _signalEvent{nullptr};
#else
    static void _intSignalHandler(int signum);
    static void _termSignalHandler(int signum);

    int _sigIntFd[2] = {-1, -1};   // socketpair read=0, write=1
    int _sigTermFd[2] = {-1, -1};
    QSocketNotifier* _notifierInt{nullptr};
    QSocketNotifier* _notifierTerm{nullptr};
#endif

    bool _sigIntTriggered{false};
    static std::atomic<SignalHandler*> s_current;
};
