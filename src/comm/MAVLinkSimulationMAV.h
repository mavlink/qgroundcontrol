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
    explicit MAVLinkSimulationMAV(MAVLinkSimulationLink *parent, int systemid);

signals:

public slots:
    void mainloop();
    void handleMessage(const mavlink_message_t& msg);

protected:
    MAVLinkSimulationLink* link;
    MAVLinkSimulationWaypointPlanner planner;
    int systemid;
    QTimer mainloopTimer;


};

#endif // MAVLINKSIMULATIONMAV_H
