#pragma once

#include <deque>
#include <functional>

#include "GPSConnectionState.h"
#include "GPSReceiverProfile.h"
#include "ScheduledTask.h"

/// Shared intent, admission, command notifications, and retry scheduling; backends own acquisition and teardown.
class GPSConnectionControl : public QObject
{
    Q_OBJECT
public:
    enum class NotificationPolicy
    {
        Immediate,
        AfterCommands
    };
    GPSConnectionControl(NotificationPolicy policy, QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                         GPSReceiverProfile profile = {});
    ~GPSConnectionControl() override;

    GPSConnectionState& connection() { return _connection; }

    const GPSConnectionState& connection() const { return _connection; }

    RuntimeScheduler* scheduler() const { return _scheduler; }

    const GPSReceiverProfile& profile() const { return _profile; }

    bool automatic() const { return _automatic; }

    bool suspended() const { return _suspended; }

    bool stopped() const { return _stopped; }

    bool shutdown() const { return _shutdown; }

    quint64 revision() const { return _revision; }

    quint64 beginCommand() { return ++_revision; }

    bool changeProfile(const GPSReceiverProfile& profile);
    bool changeAutomatic(bool enabled);
    bool changeSuspended(bool suspended);

    void setStopped(bool stopped) { _stopped = stopped; }

    void setShutdown()
    {
        _shutdown = true;
        _update.cancel();
    }

    bool shouldConnect() const;
    bool startAttempt(std::function<bool()> start);

    void dispatch(std::function<void()> command);
    void enqueue(std::function<void()> command);
    void notifyChanged();

    void cancelUpdate() { _update.cancel(); }

    void scheduleUpdate(bool backendIdle, qint64 discoveryPollMs, bool deferInitialDiscovery,
                        std::function<void()> update);

signals:
    void changed();
    void commandsDrained();

private:
    NotificationPolicy _policy;
    QPointer<RuntimeScheduler> _scheduler;
    GPSConnectionState _connection;
    ScheduledTask _update;
    GPSReceiverProfile _profile;
    bool _automatic = false;
    bool _suspended = false;
    bool _stopped = false;
    bool _shutdown = false;
    quint64 _revision = 0;
    bool _dispatching = false;
    bool _notificationPending = false;
    std::deque<std::function<void()>> _commands;
};
