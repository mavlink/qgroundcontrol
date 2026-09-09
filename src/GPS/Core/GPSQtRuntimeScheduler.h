#pragma once

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include "GPSRuntimeScheduler.h"

class QChronoTimer;

/// Qt event-loop implementation; no callback runs inside schedule().
class GPSQtRuntimeScheduler final : public GPSRuntimeScheduler
{
    Q_OBJECT
public:
    explicit GPSQtRuntimeScheduler(QObject* parent = nullptr);
    ~GPSQtRuntimeScheduler() override;
    quint64 nowUs() const override;
    TaskId schedule(QObject* context, std::chrono::microseconds delay, Callback callback) override;
    void cancel(TaskId task) override;

private:
    TaskId _nextTask = 0;
    QHash<TaskId, QPointer<QChronoTimer>> _tasks;
};
