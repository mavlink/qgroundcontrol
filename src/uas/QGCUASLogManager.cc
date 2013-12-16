#include "QGCUASLogManager.h"

QGCUASLogManager::QGCUASLogManager(UASInterface *parent) :
    QObject(parent),
    mav(parent)
{

}

void QGCUASLogManager::receiveMessage(LinkInterface* link, mavlink_message_t message)
{

}
