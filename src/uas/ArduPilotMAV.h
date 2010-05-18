#ifndef ARDUPILOTMAV_H
#define ARDUPILOTMAV_H

#include "UAS.h"

class ArduPilotMAV : public UAS
{
public:
    ArduPilotMAV(MAVLinkProtocol* mavlink, int id = 0);
};

#endif // ARDUPILOTMAV_H
