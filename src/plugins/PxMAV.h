#ifndef PXMAV_H
#define PXMAV_H

#include "UAS.h"

class PxMAV : public UAS
{
    Q_OBJECT
    Q_INTERFACES(UASInterface)
public:
    PxMAV(MAVLinkProtocol* mavlink, int id);
    PxMAV();
public slots:
    /** @brief Receive a MAVLink message from this MAV */
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    /** @brief Send a command to an onboard process */
    void sendProcessCommand(int watchdogId, int processId, unsigned int command);
signals:
    void watchdogReceived(int systemId, int watchdogId, unsigned int processCount);
    void processReceived(int systemId, int watchdogId, int processId, QString name, QString arguments, int timeout);
    void processChanged(int systemId, int watchdogId, int processId, int state, bool muted, int crashed, int pid);
};

#endif // PXMAV_H
