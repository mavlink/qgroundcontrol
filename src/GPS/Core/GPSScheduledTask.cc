#include "GPSScheduledTask.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSScheduledTaskLog, "GPS.Core.GPSScheduledTask")

GPSScheduledTask::GPSScheduledTask(GPSRuntimeScheduler* scheduler, QObject* context)
    : _scheduler(scheduler)
    , _context(context)
    , _state(std::make_shared<State>())
{
    qCDebug(GPSScheduledTaskLog) << this;
}

GPSScheduledTask::~GPSScheduledTask()
{
    qCDebug(GPSScheduledTaskLog) << this;
    cancel();
}

bool GPSScheduledTask::schedule(std::chrono::microseconds delay, GPSRuntimeScheduler::Callback callback)
{
    cancel();
    if (!_scheduler || !_context || !callback) {
        return false;
    }
    const auto generation = _state->generation;
    const std::weak_ptr<State> state = _state;
    _state->id = _scheduler->schedule(_context, delay, [state, generation, callback = std::move(callback)]() {
        const auto current = state.lock();
        if (!current || current->generation != generation) {
            return;
        }
        current->id = 0;
        callback();
    });
    return _state->id != 0;
}

void GPSScheduledTask::cancel()
{
    ++_state->generation;
    const auto id = std::exchange(_state->id, 0);
    if (_scheduler && id) {
        _scheduler->cancel(id);
    }
}

bool GPSScheduledTask::active() const
{
    return _scheduler && _context && _state->id != 0;
}
