#pragma once

#include <QtCore/QPointer>

#include <memory>

#include "GPSRuntimeScheduler.h"

/// Owns one deferred callback on the scheduler's thread; replacing or destroying it cancels the callback.
class GPSScheduledTask
{
public:
    GPSScheduledTask(GPSRuntimeScheduler* scheduler, QObject* context);
    ~GPSScheduledTask();

    Q_DISABLE_COPY_MOVE(GPSScheduledTask)

    bool schedule(std::chrono::microseconds delay, GPSRuntimeScheduler::Callback callback);
    void cancel();
    bool active() const;

private:
    struct State
    {
        GPSRuntimeScheduler::TaskId id = 0;
        quint64 generation = 0;
    };

    QPointer<GPSRuntimeScheduler> _scheduler;
    QPointer<QObject> _context;
    std::shared_ptr<State> _state;
};
