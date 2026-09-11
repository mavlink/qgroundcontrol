#pragma once

#include <QtCore/QPointer>

#include <memory>

#include "RuntimeScheduler.h"

/// Owns one deferred callback on the scheduler's thread; replacing or destroying it cancels the callback.
class ScheduledTask
{
public:
    ScheduledTask(RuntimeScheduler* scheduler, QObject* context);
    ~ScheduledTask();

    Q_DISABLE_COPY_MOVE(ScheduledTask)

    bool schedule(std::chrono::microseconds delay, RuntimeScheduler::Callback callback);
    void cancel();
    bool active() const;

private:
    struct State
    {
        RuntimeScheduler::TaskId id = 0;
        quint64 generation = 0;
    };

    QPointer<RuntimeScheduler> _scheduler;
    QPointer<QObject> _context;
    std::shared_ptr<State> _state;
};
