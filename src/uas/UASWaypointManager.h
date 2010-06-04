#ifndef UASWAYPOINTMANAGER_H
#define UASWAYPOINTMANAGER_H

#include <QObject>
#include "Waypoint.h"
#include <mavlink.h>
class UAS;

class UASWaypointManager : public QObject
{
Q_OBJECT
private:
    enum WaypointState {
        WP_IDLE = 0,
        WP_SENDLIST,
        WP_SENDLIST_SENDWPS,
        WP_GETLIST,
        WP_GETLIST_GETWPS
    }; ///< The possible states for the waypoint protocol

public:
    UASWaypointManager(UAS&);

    void handleWaypointCount(quint8 systemId, quint8 compId, quint16 count);
    void handleWaypoint(quint8 systemId, quint8 compId, mavlink_waypoint_t *wp);

private:
    void getWaypoint(quint16 seq);

public slots:
    void waypointChanged(Waypoint*);
    void currentWaypointChanged(int);
    void removeWaypointId(int);
    void requestWaypoints();
    void clearWaypointList();

signals:
    void waypointUpdated(int,int,double,double,double,double,bool,bool);

private:
    UAS &uas;
    quint16 current_wp_id;                 ///< The last used waypoint ID
    quint16 current_count;
    WaypointState current_state;        ///< The current state
    quint8 current_partner_systemid;
    quint8 current_partner_compid;
};

#endif // UASWAYPOINTMANAGER_H
