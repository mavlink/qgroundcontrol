#ifndef UASWAYPOINTMANAGER_H
#define UASWAYPOINTMANAGER_H

#include <QObject>
#include <QVector>
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
    void handleWaypointRequest(quint8 systemId, quint8 compId, mavlink_waypoint_request_t *wpr);

private:
    void sendWaypointRequest(quint16 seq);

public slots:
    void clearWaypointList();
    void currentWaypointChanged(quint16);
    void removeWaypointId(quint16);
    void requestWaypoints();
    void sendWaypoints(const QVector<Waypoint *> &list);
    void waypointChanged(Waypoint*);

signals:
    void waypointUpdated(int,quint16,double,double,double,double,bool,bool);    ///< Adds a waypoint to the waypoint list widget
    void updateStatusString(const QString &);                         ///< updates the current status string

private:
    UAS &uas;                                       ///< Reference to the corresponding UAS
    quint16 current_wp_id;                          ///< The last used waypoint ID in the current protocol transaction
    quint16 current_count;                          ///< The number of waypoints in the current protocol transaction
    WaypointState current_state;                    ///< The current protocol state
    quint8 current_partner_systemid;                ///< The current protocol communication target system
    quint8 current_partner_compid;                  ///< The current protocol communication target component

    QVector<mavlink_waypoint_t *> waypoint_buffer;  ///< communication buffer for waypoints
};

#endif // UASWAYPOINTMANAGER_H
