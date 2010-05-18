#ifndef ARDUPILOTMAV_H
#define ARDUPILOTMAV_H

#include "UAS.h"

class ArduPilotMAV : public UAS
{
    Q_OBJECT
public:
    ArduPilotMAV(MAVLinkProtocol* mavlink, int id = 0);
};

#endif // ARDUPILOTMAV_H
