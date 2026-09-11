#include "ScheduledTask.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ScheduledTaskLog, "Utilities.Timing.ScheduledTask")

ScheduledTask::ScheduledTask(RuntimeScheduler* scheduler, QObject* context)
    : _scheduler(scheduler)
    , _context(context)
    , _state(std::make_shared<State>())
{
    qCDebug(ScheduledTaskLog) << this;
}

ScheduledTask::~ScheduledTask()
{
    qCDebug(ScheduledTaskLog) << this;
    cancel();
}

bool ScheduledTask::schedule(std::chrono::microseconds delay, RuntimeScheduler::Callback callback)
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

void ScheduledTask::cancel()
{
    ++_state->generation;
    const auto id = std::exchange(_state->id, 0);
    if (_scheduler && id) {
        _scheduler->cancel(id);
    }
}

bool ScheduledTask::active() const
{
    return _scheduler && _context && _state->id != 0;
}
