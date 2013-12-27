#ifndef QGCUASLOGMANAGER_H
#define QGCUASLOGMANAGER_H

#include <QObject>
#include "comm/MAVLinkProtocol.h"
#include "comm/LinkInterface.h"

class UASInterface;

class QGCUASLogManager : public QObject
{
    Q_OBJECT
public:
    explicit QGCUASLogManager(UASInterface *parent = 0);
    
signals:
    
public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void requestLogList();
    void deleteLogs();

protected:
    UASInterface* mav;
    
};

#endif // QGCUASLOGMANAGER_H
