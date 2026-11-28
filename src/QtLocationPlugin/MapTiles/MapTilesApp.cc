#include "MapTilesApp.h"

#include "AppMessages.h"

namespace MapTilesApp {

void showMessage(const QString& message)
{
    QGC::showAppMessage(message);
}

bool runningUnitTests()
{
    return QGC::runningUnitTests();
}

}  // namespace MapTilesApp
