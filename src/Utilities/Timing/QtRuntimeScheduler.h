#pragma once

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include "RuntimeScheduler.h"

class QChronoTimer;

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

private:
    TaskId _nextTask = 0;
    QHash<TaskId, QPointer<QChronoTimer>> _tasks;
};
