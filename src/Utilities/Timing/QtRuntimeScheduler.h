#pragma once

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include "RuntimeScheduler.h"

/// Qt event-loop implementation; no callback runs inside schedule().
class QtRuntimeScheduler final : public RuntimeScheduler
{
    Q_OBJECT
public:
    explicit QtRuntimeScheduler(QObject* parent = nullptr);
    ~QtRuntimeScheduler() override;
    quint64 nowUs() const override;
    TaskId schedule(QObject* context, std::chrono::microseconds delay, Callback callback) override;
    void cancel(TaskId task) override;

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    struct Task
    {
        TaskId id = 0;
        QPointer<QObject> context;
        Callback callback;
        QMetaObject::Connection contextDestroyed;
    };

    TaskId _nextTask = 0;
    QHash<int, Task> _tasks;
    QHash<TaskId, int> _timers;
};
