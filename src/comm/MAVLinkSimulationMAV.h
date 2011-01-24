#ifndef MAVLINKSIMULATIONMAV_H
#define MAVLINKSIMULATIONMAV_H

#include <QObject>
#include <QTimer>

#include "MAVLinkSimulationLink.h"
#include "MAVLinkSimulationWaypointPlanner.h"

class MAVLinkSimulationMAV : public QObject
{
    Q_OBJECT
public:
    explicit MAVLinkSimulationMAV(MAVLinkSimulationLink *parent, int systemid, double lat=47.376389, double lon=8.548056);

signals:

public slots:
    void mainloop();
    void handleMessage(const mavlink_message_t& msg);

protected:
    MAVLinkSimulationLink* link;
    MAVLinkSimulationWaypointPlanner planner;
    int systemid;
    QTimer mainloopTimer;
    int timer25Hz;
    int timer10Hz;
    int timer1Hz;
    double latitude;
    double longitude;
    double altitude;
    double x;
    double y;
    double z;
    double roll;
    double pitch;
    double yaw;

    bool globalNavigation;
    bool firstWP;

    double previousSPX;
    double previousSPY;
    double previousSPZ;
    double previousSPYaw;

    double nextSPX;
    double nextSPY;
    double nextSPZ;
    double nextSPYaw;
    uint8_t sys_mode;
    uint8_t sys_state;
    uint8_t nav_mode;
    bool flying;

};

#endif // MAVLINKSIMULATIONMAV_H
