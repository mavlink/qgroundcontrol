#include "ManualScheduler.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ManualSchedulerLog, "Utilities.Timing.ManualScheduler")

ManualScheduler::ManualScheduler(QObject* parent, quint64 initialUs)
    : RuntimeScheduler(parent)
    , _nowUs(initialUs)
{
    qCDebug(ManualSchedulerLog) << this;
}

ManualScheduler::~ManualScheduler()
{
    qCDebug(ManualSchedulerLog) << this;
}

RuntimeScheduler::TaskId ManualScheduler::schedule(QObject* context, std::chrono::microseconds delay, Callback callback)
{
    if (!context || !callback || context->thread() != thread()) {
        return 0;
    }
    const auto delta = static_cast<quint64>((std::max) (delay, std::chrono::microseconds::zero()).count());
    if (delta > (std::numeric_limits<quint64>::max)() - _nowUs) {
        return 0;
    }
    const auto id = ++_nextTask;
    _tasks.emplace(std::make_pair(_nowUs + delta, id), Task{context, std::move(callback)});
    return id;
}

void ManualScheduler::cancel(TaskId task)
{
    for (auto it = _tasks.begin(); it != _tasks.end(); ++it) {
        if (it->first.second == task) {
            _tasks.erase(it);
            return;
        }
    }
}

bool ManualScheduler::advanceToUs(quint64 targetUs)
{
    if (_advancing || targetUs < _nowUs) {
        return false;
    }
    _advancing = true;
    const QPointer<ManualScheduler> guard(this);
    int dispatched = 0;
    while (!_tasks.empty() && _tasks.begin()->first.first <= targetUs) {
        if (++dispatched > 100000) {
            _advancing = false;
            return false;
        }
        auto node = _tasks.extract(_tasks.begin());
        _nowUs = node.key().first;
        auto task = std::move(node.mapped());
        if (task.context) {
            task.callback();
        }
        if (!guard) {
            return false;
        }
    }
    _nowUs = targetUs;
    _advancing = false;
    return true;
}

bool ManualScheduler::advanceBy(std::chrono::microseconds duration)
{
    return duration.count() >= 0 &&
           static_cast<quint64>(duration.count()) <= (std::numeric_limits<quint64>::max)() - _nowUs &&
           advanceToUs(_nowUs + static_cast<quint64>(duration.count()));
}

qsizetype ManualScheduler::pendingCount() const
{
    return std::count_if(_tasks.cbegin(), _tasks.cend(),
                         [](const auto& task) { return !task.second.context.isNull(); });
}
