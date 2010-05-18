#ifndef SLUGSMAV_H
#define SLUGSMAV_H

#include "UAS.h"

class SlugsMAV : public UAS
{
public:
    SlugsMAV(MAVLinkProtocol* mavlink, int id = 0);

public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
};

#endif // SLUGSMAV_H
