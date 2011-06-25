#ifndef QGCMAPWIDGET_H
#define QGCMAPWIDGET_H

#include <QMap>
#include "opmapcontrol.h"

class UASInterface;
class UASWaypointManager;
class Waypoint;

/**
 * @brief Class representing a 2D map using aerial imagery
 */
class QGCMapWidget : public mapcontrol::OPMapWidget
{
    Q_OBJECT
public:
    explicit QGCMapWidget(QWidget *parent = 0);
    ~QGCMapWidget();

signals:
    void homePositionChanged(double latitude, double longitude, double altitude);

public slots:
    /** @brief Add system to map view */
    void addUAS(UASInterface* uas);
    /** @brief Update the global position of a system */
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);
    /** @brief Update the type, size, etc. of this system */
    void updateSystemSpecs(int uas);
    /** @brief Change current system in focus / editing */
    void activeUASSet(UASInterface* uas);
    /** @brief Show a dialog to jump to given GPS coordinates */
    void showGoToDialog();
    /** @brief Update this waypoint for this UAS */
    void updateWaypoint(int uas, Waypoint* wp);
    /** @brief Update the whole waypoint */
    void updateWaypointList(int uas);
    /** @brief Update the home position on the map */
    void updateHomePosition(double latitude, double longitude, double altitude);

protected:
    /** @brief Update the highlighting of the currently controlled system */
    void updateSelectedSystem(int uas);
    /** @brief Set the current mouse position on the map as new home position */

    UASWaypointManager* currWPManager; ///< The current waypoint manager
    QMap<Waypoint* , mapcontrol::WayPointItem*> waypointsToIcons;
    QMap<mapcontrol::WayPointItem*, Waypoint*> iconsToWaypoints;
//    enum editMode {
//        NONE,
//        WAYPOINTS,
//        SWEEP,
//        UAVS,
//        HOME,
//        SAFE_AREA
//    };
//    enum editMode currEditMode;            ///< The current edit mode on the map

};

#endif // QGCMAPWIDGET_H
