#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <functional>

#include "GPSConnectionError.h"
#include "RuntimeScheduler.h"

/// Connection intent, lifecycle, and retry policy, mutated by one controller.
class GPSConnectionState : public QObject
{
    Q_OBJECT

    friend class GPSConnectionStateTest;
    friend class NMEASourceManagerTest;
    friend class GPSReceiverAutoConnectTest;

public:
    enum State
    {
        Disconnected,
        Connecting,
        Configuring,
        Ready,
        Retrying,
        Stopping,
        AwaitingChange,
    };
    Q_ENUM(State)

    explicit GPSConnectionState(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSConnectionState() override;

    State state() const { return _state; }

    bool active() const { return _active; }

    bool paused() const { return _paused; }

    bool shouldConnect(bool automatic) const;
    bool updateIntent(bool automatic);
    void requestConnect();
    void pause();
    void stop();
    void resetIntent();

    bool canAttempt() const;
    qint64 retryRemainingMs() const;
    bool beginAttempt();
    /// Commit backend startup, or retire an unchanged admission when startup was superseded.
    bool startAttempt(const std::function<bool()>& start);
    void configuring();
    void ready();
    void failed(GPSRetryDisposition disposition = GPSRetryDisposition::Retry);
    void stopping();
    void stopped();
    void resetRetry();

signals:
    void changed();

private:
    void _setState(State state);

    State _state = Disconnected;
    bool _active = false;
    bool _manualRequested = false;
    bool _paused = false;
    QPointer<RuntimeScheduler> _scheduler;
    qint64 _retryDeadlineMs = -1;
    int _retryDelayMs = 1000;
    quint64 _transitionRevision = 0;
};
