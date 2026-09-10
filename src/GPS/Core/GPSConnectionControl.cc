#include "GPSConnectionControl.h"

#include <algorithm>
#include <utility>

#include "GPSQtRuntimeScheduler.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSConnectionControlLog, "GPS.Core.GPSConnectionControl")

GPSConnectionControl::GPSConnectionControl(NotificationPolicy policy, QObject* parent, GPSRuntimeScheduler* scheduler,
                                           GPSReceiverProfile profile)
    : QObject(parent)
    , _policy(policy)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _connection(this, _scheduler)
    , _update(_scheduler, this)
    , _profile(profile.normalized())
{
    qCDebug(GPSConnectionControlLog) << this;
    connect(&_connection, &GPSConnectionState::changed, this, &GPSConnectionControl::notifyChanged);
}

GPSConnectionControl::~GPSConnectionControl()
{
    qCDebug(GPSConnectionControlLog) << this;
}

bool GPSConnectionControl::changeProfile(const GPSReceiverProfile& profile)
{
    const auto normalized = profile.normalized();
    if (_profile == normalized) {
        return false;
    }
    ++_revision;
    _profile = normalized;
    _stopped = false;
    return true;
}

bool GPSConnectionControl::changeAutomatic(bool enabled)
{
    if (_automatic == enabled) {
        return false;
    }
    ++_revision;
    _automatic = enabled;
    _stopped = false;
    return true;
}

bool GPSConnectionControl::changeSuspended(bool suspended)
{
    if (std::exchange(_suspended, suspended) == suspended) {
        return false;
    }
    ++_revision;
    return true;
}

bool GPSConnectionControl::shouldConnect() const
{
    return !_shutdown && _connection.shouldConnect(_automatic) &&
           _profile.endpoint.kind != GPSReceiverProfile::Endpoint::Kind::Disabled;
}

bool GPSConnectionControl::startAttempt(std::function<bool()> start)
{
    const auto revision = _revision;
    const QPointer<GPSConnectionControl> guard(this);
    return _connection.startAttempt([this, guard, revision, start = std::move(start)]() {
        return guard && revision == _revision && !_suspended && !_shutdown && start && start();
    });
}

void GPSConnectionControl::dispatch(std::function<void()> command)
{
    if (_policy == NotificationPolicy::Immediate) {
        command();
        return;
    }
    _commands.push_back(std::move(command));
    if (_dispatching) {
        return;
    }
    _dispatching = true;
    const QPointer<GPSConnectionControl> guard(this);
    while (!_commands.empty() || _notificationPending) {
        if (!_commands.empty()) {
            auto next = std::move(_commands.front());
            _commands.pop_front();
            next();
        } else {
            _notificationPending = false;
            emit changed();
        }
        if (!guard) {
            return;
        }
    }
    _dispatching = false;
    emit commandsDrained();
}

void GPSConnectionControl::enqueue(std::function<void()> command)
{
    if (_policy == NotificationPolicy::Immediate) {
        dispatch(std::move(command));
        return;
    }
    _commands.push_back(std::move(command));
    if (!_dispatching) {
        dispatch([]() {});
    }
}

void GPSConnectionControl::notifyChanged()
{
    if (_policy == NotificationPolicy::Immediate) {
        emit changed();
        return;
    }
    _notificationPending = true;
    if (!_dispatching) {
        dispatch([]() {});
    }
}

void GPSConnectionControl::scheduleUpdate(bool backendIdle, qint64 discoveryPollMs, bool deferInitialDiscovery,
                                          std::function<void()> update)
{
    _update.cancel();
    if (!_scheduler || _stopped || _suspended || !shouldConnect()) {
        return;
    }
    qint64 delay = backendIdle ? _connection.retryRemainingMs() : -1;
    if (backendIdle && delay < 0 && _connection.state() == GPSConnectionState::Disconnected) {
        delay = 0;
    }
    if (discoveryPollMs >= 0) {
        delay = delay < 0 ? discoveryPollMs : (std::min) (delay, discoveryPollMs);
        if (deferInitialDiscovery && delay == 0 && _connection.state() == GPSConnectionState::Disconnected) {
            delay = discoveryPollMs;
        }
    }
    if (delay >= 0) {
        _update.schedule(std::chrono::milliseconds(delay), std::move(update));
    }
}
