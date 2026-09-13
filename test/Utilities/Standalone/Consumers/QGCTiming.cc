#include <QtCore/QCoreApplication>

#include "ManualScheduler.h"
#include "QtRuntimeScheduler.h"
#include "ScheduledTask.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ManualScheduler manual;
    QtRuntimeScheduler runtime;
    ScheduledTask task(&manual, &app);
    bool called = false;
    if (!task.schedule(std::chrono::microseconds::zero(), [&] { called = true; }))
        return 1;
    return manual.advanceBy(std::chrono::microseconds::zero()) && called ? 0 : 2;
}
