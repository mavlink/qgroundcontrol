#include "QGCUASFileManager.h"
#include "QGC.h"

QGCUASFileManager::QGCUASFileManager(QObject* parent, UASInterface* uas) :
    QObject(parent),
    _mav(uas)
{
}

void QGCUASFileManager::nothingMessage() {
    mavlink_message_t message;

    _mav->sendMessage(message);
}
