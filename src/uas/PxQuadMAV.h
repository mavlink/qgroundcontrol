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
};

#endif // PXQUADMAV_H
