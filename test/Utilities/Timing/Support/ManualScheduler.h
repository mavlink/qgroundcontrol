#pragma once

#include <QtCore/QPointer>

#include <map>

#include "RuntimeScheduler.h"

/// Deterministic event scheduler. Callbacks run in deadline/id order without sleeping or pumping unrelated events.
class ManualScheduler final : public RuntimeScheduler
{
    Q_OBJECT
public:
    explicit ManualScheduler(QObject* parent = nullptr, quint64 initialUs = 1000000);
    ~ManualScheduler() override;

    quint64 nowUs() const override { return _nowUs; }

    TaskId schedule(QObject* context, std::chrono::microseconds delay, Callback callback) override;
    void cancel(TaskId task) override;
    bool advanceToUs(quint64 targetUs);
    bool advanceBy(std::chrono::microseconds duration);
    qsizetype pendingCount() const;

private:
    struct Task
    {
        QPointer<QObject> context;
        Callback callback;
    };

    quint64 _nowUs;
    TaskId _nextTask = 0;
    std::map<std::pair<quint64, TaskId>, Task> _tasks;
    bool _advancing = false;
};
