#include "QGCUASLogManager.h"

#include "uas/UAS.h"

QGCUASLogManager::QGCUASLogManager(UASInterface *parent) :
    QObject(parent),
    mav(parent)
{
    // Play dumb for testing and always request logs
    requestLogList();
}

void QGCUASLogManager::receiveMessage(LinkInterface* link, mavlink_message_t message)
{

    switch (message.msgid) {
        case MAVLINK_MSG_ID_LOG_ENTRY:
    {
        mavlink_log_entry_t log;
        mavlink_msg_log_entry_decode(&message, &log);
        qDebug() << "RECEIVED LOG ENTRY MESSAGE: #" << log.id << "SIZE:" << log.size;
    }
        break;

    }
}

void QGCUASLogManager::requestLogList()
{
    UAS* uas = qobject_cast<UAS*>(mav);
    if (uas) {
        uas->requestLogList();
        qDebug() << "REQUESTED LOGS";
    }
}

void QGCUASLogManager::deleteLogs()
{
    UAS* uas = qobject_cast<UAS*>(mav);
    if (uas)
        uas->deleteLogs();
}
