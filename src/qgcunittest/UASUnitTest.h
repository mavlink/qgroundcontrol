#ifndef UASUNITTEST_H
#define UASUNITTEST_H

#include "UAS.h"
#include "MAVLinkProtocol.h"
#include "SerialLink.h"
#include "UASInterface.h"
#include "UnitTest.h"
#include "LinkManager.h"
#include "UASWaypointManager.h"
#include "SerialLink.h"
#include "LinkInterface.h"

#define  UASID  100

class UASUnitTest : public UnitTest
{
    Q_OBJECT
    
public:
    UASUnitTest(void);

private slots:
    void init(void);
    void cleanup(void);

    void getUASID_test(void);
    void getUASName_test(void);
    void getUpTime_test(void);
    void filterVoltage_test(void);
    void getAutopilotType_test(void);
    void setAutopilotType_test(void);
    void getStatusForCode_test(void);
    void getLocalX_test(void);
    void getLocalY_test(void);
    void getLocalZ_test(void);
    void getLatitude_test(void);
    void getLongitude_test(void);
    void getAltitudeAMSL_test(void);
    void getAltitudeRelative_test(void);
    void getRoll_test(void);
    void getPitch_test(void);
    void getYaw_test(void);
    void getSelected_test(void);
    void getSystemType_test(void);
    void getAirframe_test(void);
    void setAirframe_test(void);
    void getWaypointList_test(void);
    void signalWayPoint_test(void);
    void getWaypoint_test(void);
    
private:
    MAVLinkProtocol*    _mavlink;
    UAS*                _uas;
};

#endif
