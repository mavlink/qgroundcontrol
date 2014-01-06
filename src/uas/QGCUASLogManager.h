#ifndef QGCUASLOGMANAGER_H
#define QGCUASLOGMANAGER_H

#include <QObject>
#include <QSet>
#include "comm/MAVLinkProtocol.h"
#include "comm/LinkInterface.h"

class UASInterface;

class QGCUASLogManager : public QObject
{
    Q_OBJECT
public:
    explicit QGCUASLogManager(UASInterface *parent = 0);
    
signals:
    void logEntryReceived(unsigned int id, unsigned int size, quint64 timestamp);
    void logEntryFileReceived(unsigned int id, QByteArray data);
    
public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void requestLogList();
    void requestLogData(unsigned int id);
    void deleteLogs();

protected:
    UASInterface* mav;
    
};

#endif // QGCUASLOGMANAGER_H
