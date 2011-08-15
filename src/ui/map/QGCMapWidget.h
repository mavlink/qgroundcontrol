#ifndef QGCMAPWIDGET_H
#define QGCMAPWIDGET_H

#include <QMap>
#include <QTimer>
#include "opmapcontrol.h"

class UASInterface;
class UASWaypointManager;
class Waypoint;
typedef mapcontrol::WayPointItem WayPointItem;

/**
 * @brief Class representing a 2D map using aerial imagery
 */
class QGCMapWidget : public mapcontrol::OPMapWidget
{
    Q_OBJECT
public:
    explicit QGCMapWidget(QWidget *parent = 0);
    ~QGCMapWidget();
//    /** @brief Convert meters to pixels */
//    float metersToPixels(double meters);
//    double headingP1P2(internals::PointLatLng p1, internals::PointLatLng p2);
//    internals::PointLatLng targetLatLon(internals::PointLatLng source, double heading, double dist);

    /** @brief Map centered on current active system */
    bool getFollowUAVEnabled() { return followUAVEnabled; }
    /** @brief The maximum map update rate */
    float getUpdateRateLimit() { return maxUpdateInterval; }
    /** @brief Get the trail type */
    int getTrailType() { return static_cast<int>(trailType); }
    /** @brief Get the trail interval */
    float getTrailInterval() { return trailInterval; }

signals:
    void homePositionChanged(double latitude, double longitude, double altitude);
    /** @brief Signal for newly created map waypoints */
    void waypointCreated(Waypoint* wp);
    void waypointChanged(Waypoint* wp);

public slots:
    /** @brief Add system to map view */
    void addUAS(UASInterface* uas);
    /** @brief Update the global position of a system */
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);
    /** @brief Update the global position of all systems */
    void updateGlobalPosition();
    /** @brief Update the type, size, etc. of this system */
    void updateSystemSpecs(int uas);
    /** @brief Change current system in focus / editing */
    void activeUASSet(UASInterface* uas);
    /** @brief Show a dialog to jump to given GPS coordinates */
    void showGoToDialog();
    /** @brief Jump to the home position on the map */
    void goHome();
    /** @brief Update this waypoint for this UAS */
    void updateWaypoint(int uas, Waypoint* wp);
    /** @brief Update the whole waypoint */
    void updateWaypointList(int uas);
    /** @brief Update the home position on the map */
    void updateHomePosition(double latitude, double longitude, double altitude);
    /** @brief Set update rate limit */
    void setUpdateRateLimit(float seconds);
    /** @brief Cache visible region to harddisk */
    void cacheVisibleRegion();
    /** @brief Set follow mode */
    void setFollowUAVEnabled(bool enabled) { followUAVEnabled = enabled; }
    /** @brief Set trail to time mode and set time @param seconds The minimum time between trail dots in seconds. If set to a value < 0, trails will be disabled*/
    void setTrailModeTimed(int seconds)
    {
        foreach(mapcontrol::UAVItem* uav, GetUAVS())
        {
            if (seconds >= 0)
            {
                uav->SetTrailTime(seconds);
                uav->SetTrailType(mapcontrol::UAVTrailType::ByTimeElapsed);
            }
            else
            {
                uav->SetTrailType(mapcontrol::UAVTrailType::NoTrail);
            }
        }
    }
    /** @brief Set trail to distance mode and set time @param meters The minimum distance between trail dots in meters. The actual distance depends on the MAV's update rate as well. If set to a value < 0, trails will be disabled*/
    void setTrailModeDistance(int meters)
    {
        foreach(mapcontrol::UAVItem* uav, GetUAVS())
        {
            if (meters >= 0)
            {
                uav->SetTrailDistance(meters);
                uav->SetTrailType(mapcontrol::UAVTrailType::ByDistance);
            }
            else
            {
                uav->SetTrailType(mapcontrol::UAVTrailType::NoTrail);
            }
        }
    }
    /** @brief Delete all trails */
    void deleteTrails()
    {
        foreach(mapcontrol::UAVItem* uav, GetUAVS())
        {
            uav->DeleteTrail();
        }
    }

    /** @brief Load the settings for this widget from disk */
    void loadSettings(bool changePosition=true);
    /** @brief Store the settings for this widget to disk */
    void storeSettings();

protected slots:
    /** @brief Convert a map edit into a QGC waypoint event */
    void handleMapWaypointEdit(WayPointItem* waypoint);

protected:
    /** @brief Update the highlighting of the currently controlled system */
    void updateSelectedSystem(int uas);
    /** @brief Initialize */
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);

    UASWaypointManager* currWPManager; ///< The current waypoint manager
    QMap<Waypoint* , mapcontrol::WayPointItem*> waypointsToIcons;
    QMap<mapcontrol::WayPointItem*, Waypoint*> iconsToWaypoints;
    Waypoint* firingWaypointChange;
    QTimer updateTimer;
    float maxUpdateInterval;
    enum editMode {
        EDIT_MODE_NONE,
        EDIT_MODE_WAYPOINTS,
        EDIT_MODE_SWEEP,
        EDIT_MODE_UAVS,
        EDIT_MODE_HOME,
        EDIT_MODE_SAFE_AREA,
        EDIT_MODE_CACHING
    };
    editMode currEditMode;            ///< The current edit mode on the map
    bool followUAVEnabled;              ///< Does the map follow the UAV?
    mapcontrol::UAVTrailType::Types trailType; ///< Time or distance based trail dots
    float trailInterval;                ///< Time or distance between trail items
    int followUAVID;                    ///< Which UAV should be tracked?


};

#endif // QGCMAPWIDGET_H
