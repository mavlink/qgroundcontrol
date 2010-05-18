#ifndef PXQUADMAV_H
#define PXQUADMAV_H

#include "UAS.h"

class PxQuadMAV : public UAS
{
    Q_OBJECT
public:
    PxQuadMAV(MAVLinkProtocol* mavlink, int id);
};

#endif // PXQUADMAV_H
