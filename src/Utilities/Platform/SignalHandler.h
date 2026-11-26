#pragma once

#include <atomic>

#include <QtCore/QObject>

#ifdef Q_OS_WIN
    class QWinEventNotifier;
#else
    class QSocketNotifier;
#endif

class SignalHandler final : public QObject
{
    Q_OBJECT

public:
    explicit SignalHandler(QObject* parent = nullptr);
    ~SignalHandler() override;

    int setupSignalHandlers();

    static SignalHandler* current() { return s_current.load(std::memory_order_acquire); }

signals:
    /// The process was told to terminate (SIGTERM). Nobody may be there to answer a question, so whoever takes this
    /// closes the application without one. Unconnected, the application is asked to quit as for an interrupt.
    void terminateRequested();

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
