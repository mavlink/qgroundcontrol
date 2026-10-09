#include "GPSCancellation.h"

#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

struct GPSCancelToken::State
{
    std::atomic<bool> cancelled = false;
    /// Held while callbacks run, so unregistering a callback waits for a running cancel().
    std::mutex mutex;
    std::vector<const std::function<void()>*> callbacks;
};

GPSCancelToken::GPSCancelToken(std::shared_ptr<State> state)
    : _state(std::move(state))
{}

bool GPSCancelToken::isCancelled() const
{
    return _state && _state->cancelled.load(std::memory_order_acquire);
}

GPSCancelSource::GPSCancelSource()
    : _state(std::make_shared<GPSCancelToken::State>())
{}

GPSCancelToken GPSCancelSource::token() const
{
    return GPSCancelToken(_state);
}

bool GPSCancelSource::cancel()
{
    // A callback that cancels again returns here, before the mutex its own cancel() holds.
    if (_state->cancelled.load(std::memory_order_acquire)) {
        return false;
    }
    const std::lock_guard lock(_state->mutex);
    if (_state->cancelled.exchange(true, std::memory_order_acq_rel)) {
        return false;
    }
    for (const auto* callback : _state->callbacks) {
        (*callback)();
    }
    _state->callbacks.clear();
    return true;
}

bool GPSCancelSource::isCancelled() const
{
    return _state->cancelled.load(std::memory_order_acquire);
}

GPSCancelCallback::GPSCancelCallback(const GPSCancelToken& token, std::function<void()> callback)
    : _callback(std::move(callback))
{
    if (!token._state) {
        return;
    }
    {
        const std::lock_guard lock(token._state->mutex);
        if (!token._state->cancelled.load(std::memory_order_acquire)) {
            token._state->callbacks.push_back(&_callback);
            _state = token._state;
            return;
        }
    }
    _callback();
}

GPSCancelCallback::~GPSCancelCallback()
{
    if (!_state) {
        return;
    }
    const std::lock_guard lock(_state->mutex);
    std::erase(_state->callbacks, &_callback);
}
