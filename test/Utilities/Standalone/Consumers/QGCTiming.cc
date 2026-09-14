#include <QtCore/QCoreApplication>

#include "QtRuntimeScheduler.h"
#include "ScheduledTask.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QtRuntimeScheduler runtime;
    ScheduledTask task(&runtime, &app);
    bool called = false;
    if (!task.schedule(std::chrono::microseconds::zero(), [&] { called = true; }))
        return 1;
    QCoreApplication::processEvents();
    return called ? 0 : 2;
}
