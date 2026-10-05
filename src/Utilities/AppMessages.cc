#include "AppMessages.h"

#include "QGCApplication.h"

#include <QtCore/QString>

namespace QGC {

void showAppMessage(const QString &message, const QString &title)
{
    if (auto *const app = qgcApp()) {
        app->showAppMessage(message, title);
    }
}

void showCriticalVehicleMessage(const QString &message)
{
    if (auto *const app = qgcApp()) {
        app->showCriticalVehicleMessage(message);
    }
}

void showRebootAppMessage(const QString &message, const QString &title)
{
    if (auto *const app = qgcApp()) {
        app->showRebootAppMessage(message, title);
    }
}

bool runningUnitTests()
{
    auto *const app = qgcApp();
    return app && app->runningUnitTests();
}

}
