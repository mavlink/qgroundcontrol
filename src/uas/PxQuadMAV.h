#ifndef PXQUADMAV_H
#define PXQUADMAV_H

#include "UAS.h"

class PxQuadMAV : public UAS
{
    Q_OBJECT
public:
    PxQuadMAV(MAVLinkProtocol* mavlink, int id);
public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
signals:
    void watchdogReceived(int systemId, int watchdogId, int processCount);
    void processReceived(int systemId, int watchdogId, int processId, QString name, QString arguments, int timeout);
    void processChanged(int systemId, int watchdogId, int processId, int state, bool muted, int crashed, int pid);
};

#endif // PXQUADMAV_H
