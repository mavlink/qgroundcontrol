#ifndef QGCUASLOGMANAGER_H
#define QGCUASLOGMANAGER_H

#include <QObject>
#include "uas/UASInterface.h"
#include "comm/MAVLinkProtocol.h"
#include "comm/LinkInterface.h"

class QGCUASLogManager : public QObject
{
    Q_OBJECT
public:
    explicit QGCUASLogManager(UASInterface *parent = 0);
    
signals:
    
public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);

protected:
    UASInterface* mav;
    
};

#endif // QGCUASLOGMANAGER_H
