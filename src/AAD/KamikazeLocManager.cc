#include <QApplicationStatic>
#include <QDebug>
#include "KamikazeLocManager.h"

Q_APPLICATION_STATIC(KamikazeLocManager, _kamikazeLocManager)

KamikazeLocManager* KamikazeLocManager::instance()
{
    return _kamikazeLocManager();
}