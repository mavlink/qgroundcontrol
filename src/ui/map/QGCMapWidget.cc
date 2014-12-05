#include <QInputDialog>
#include "QGCMapWidget.h"
#include "QGCMapToolBar.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "MAV2DIcon.h"
#include "Waypoint2DIcon.h"
#include "UASWaypointManager.h"
#include "QGCMessageBox.h"

QGCMapWidget::QGCMapWidget(QWidget *parent) :
    mapcontrol::OPMapWidget(parent),
    firingWaypointChange(NULL),
    maxUpdateInterval(2.1f), // 2 seconds
    followUAVEnabled(false),
    trailType(mapcontrol::UAVTrailType::ByTimeElapsed),
    trailInterval(2.0f),
    followUAVID(0),
    mapInitialized(false),
    mapPositionInitialized(false),
    homeAltitude(0),
    zoomBlocked(false),
    uas(NULL)
{
    currWPManager = UASManager::instance()->getActiveUASWaypointManager();
    waypointLines.insert(0, new QGraphicsItemGroup(map));
    connect(currWPManager, SIGNAL(waypointEditableListChanged(int)), this, SLOT(updateWaypointList(int)));
    connect(currWPManager, SIGNAL(waypointEditableChanged(int, Waypoint*)), this, SLOT(updateWaypoint(int,Waypoint*)));
    connect(this, SIGNAL(waypointCreated(Waypoint*)), currWPManager, SLOT(addWaypointEditable(Waypoint*)));
    connect(this, SIGNAL(waypointChanged(Waypoint*)), currWPManager, SLOT(notifyOfChangeEditable(Waypoint*)));
    offlineMode = true;
    // Widget is inactive until shown
    defaultGuidedAlt = -1;
    loadSettings(false);

    //handy for debugging:
    //this->SetShowTileGridLines(true);

    //default appears to be Google Hybrid, and is broken currently
#if defined MAP_DEFAULT_TYPE_BING
    this->SetMapType(MapType::BingHybrid);
#elif defined MAP_DEFAULT_TYPE_GOOGLE
    this->SetMapType(MapType::GoogleHybrid);
#else
    this->SetMapType(MapType::OpenStreetMap);
#endif

    this->setContextMenuPolicy(Qt::ActionsContextMenu);

    // Go to options
    QAction *guidedaction = new QAction(this);
    guidedaction->setText("Go To Here (Guided Mode)");
    connect(guidedaction,SIGNAL(triggered()),this,SLOT(guidedActionTriggered()));
    this->addAction(guidedaction);
    guidedaction = new QAction(this);
    guidedaction->setText("Go To Here Alt (Guided Mode)");
    connect(guidedaction,SIGNAL(triggered()),this,SLOT(guidedAltActionTriggered()));
    this->addAction(guidedaction);
    // Set home location option
    QAction *sethomeaction = new QAction(this);
    sethomeaction->setText("Set Home Location Here");
    connect(sethomeaction,SIGNAL(triggered()),this,SLOT(setHomeActionTriggered()));
    this->addAction(sethomeaction);
}
void QGCMapWidget::guidedActionTriggered()
{
    if (!uas)
    {
        QGCMessageBox::information(tr("Error"), tr("Please connect first"));
        return;
    }
    if (currWPManager)
    {
        if (defaultGuidedAlt == -1)
        {
            if (!guidedAltActionTriggered())
            {
                return;
            }
        }
        // Create new waypoint and send it to the WPManager to send out.
        internals::PointLatLng pos = map->FromLocalToLatLng(contextMousePressPos.x(), contextMousePressPos.y());
        qDebug() << "Guided action requested. Lat:" << pos.Lat() << "Lon:" << pos.Lng();
        Waypoint wp;
        wp.setLatitude(pos.Lat());
        wp.setLongitude(pos.Lng());
        wp.setAltitude(defaultGuidedAlt);
        currWPManager->goToWaypoint(&wp);
    }
}
bool QGCMapWidget::guidedAltActionTriggered()
{
    if (!uas)
    {
        QGCMessageBox::information(tr("Error"), tr("Please connect first"));
        return false;
    }
    bool ok = false;
    int tmpalt = QInputDialog::getInt(this,"Altitude","Enter default altitude (in meters) of destination point for guided mode",100,0,30000,1,&ok);
    if (!ok)
    {
        //Use has chosen cancel. Do not send the waypoint
        return false;
    }
    defaultGuidedAlt = tmpalt;
    guidedActionTriggered();
    return true;
}

/**
 * @brief QGCMapWidget::setHomeActionTriggered
 */
bool QGCMapWidget::setHomeActionTriggered()
{
    if (!uas)
    {
        QGCMessageBox::information(tr("Error"), tr("Please connect first"));
        return false;
    }
    UASManagerInterface *uasManager = UASManager::instance();
    if (!uasManager) { return false; }

    // Enter an altitude
    bool ok = false;
    double alt = QInputDialog::getDouble(this,"Home Altitude","Enter altitude (in meters) of new home location",0.0,0.0,30000.0,2,&ok);
    if (!ok) return false; //Use has chosen cancel. Do not send the waypoint

    // Create new waypoint and send it to the WPManager to send out.
    internals::PointLatLng pos = map->FromLocalToLatLng(contextMousePressPos.x(), contextMousePressPos.y());
    qDebug("Set home location sent. Lat: %f, Lon: %f, Alt: %f.", pos.Lat(), pos.Lng(), alt);

    bool success = uasManager->setHomePositionAndNotify(pos.Lat(),pos.Lng(), alt);

    qDebug() << ((success)? "Set new home location." : "Failed to set new home location.");

    return success;
}

void QGCMapWidget::mousePressEvent(QMouseEvent *event)
{

    // Store right-click event presses separate for context menu
    // TODO add check if click was on map, or popup box.
    if (event->button() == Qt::RightButton) {
        contextMousePressPos = event->pos();
    }

    mapcontrol::OPMapWidget::mousePressEvent(event);
}

void QGCMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mousePressPos = event->pos();
    mapcontrol::OPMapWidget::mouseReleaseEvent(event);

    // If the mouse is released, we can't be dragging
    if (firingWaypointChange) {
        firingWaypointChange->setChanged();
        firingWaypointChange = NULL;
    }
}

QGCMapWidget::~QGCMapWidget()
{
    SetShowHome(false);	// doing this appears to stop the map lib crashing on exit
    SetShowUAV(false);	//   "          "
    storeSettings();
}

void QGCMapWidget::showEvent(QShowEvent* event)
{
    // Disable OP's standard UAV, we have more than one
    SetShowUAV(false);

    // Pass on to parent widget
    OPMapWidget::showEvent(event);

    // Connect map updates to the adapter slots
    connect(this, SIGNAL(WPValuesChanged(WayPointItem*)), this, SLOT(handleMapWaypointEdit(WayPointItem*)), Qt::UniqueConnection);

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)), Qt::UniqueConnection);
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUASSet(UASInterface*)), Qt::UniqueConnection);
    connect(UASManager::instance(), SIGNAL(homePositionChanged(double,double,double)), this, SLOT(updateHomePosition(double,double,double)), Qt::UniqueConnection);

    foreach (UASInterface* uas, UASManager::instance()->getUASList())
    {
        addUAS(uas);
    }


    if (!mapInitialized)
    {
        internals::PointLatLng pos_lat_lon = internals::PointLatLng(0, 0);

        SetMouseWheelZoomType(internals::MouseWheelZoomType::MousePositionWithoutCenter);	    // set how the mouse wheel zoom functions
        SetFollowMouse(true);				    // we want a contiuous mouse position reading

        SetShowHome(true);					    // display the HOME position on the map
        Home->SetSafeArea(0);                         // set radius (meters)
        Home->SetShowSafeArea(false);                                         // show the safe area
        Home->SetCoord(pos_lat_lon);             // set the HOME position

        setFrameStyle(QFrame::NoFrame);      // no border frame
        setBackgroundBrush(QBrush(Qt::black)); // tile background

        if (!UASManager::instance()->getActiveUAS()) {
            SetCurrentPosition(pos_lat_lon);         // set the map position to default
        }

        // Set home
        updateHomePosition(UASManager::instance()->getHomeLatitude(), UASManager::instance()->getHomeLongitude(), UASManager::instance()->getHomeAltitude());

        // Set currently selected system
        activeUASSet(UASManager::instance()->getActiveUAS());
        setFocus();

        // Start timer
        connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateGlobalPosition()));
        mapInitialized = true;
        //QTimer::singleShot(800, this, SLOT(loadSettings()));
    }
    updateTimer.start(maxUpdateInterval*1000);
    // Update all UAV positions
    updateGlobalPosition();
}

void QGCMapWidget::hideEvent(QHideEvent* event)
{
    updateTimer.stop();
    storeSettings();
    OPMapWidget::hideEvent(event);
}

void QGCMapWidget::wheelEvent ( QWheelEvent * event )
{
    if (!zoomBlocked) {
        OPMapWidget::wheelEvent(event);
    }
}

/**
 * @param changePosition Load also the last position from settings and update the map position.
 */
void QGCMapWidget::loadSettings(bool changePosition)
{
    // Atlantic Ocean near Africa, coordinate origin
    double lastZoom = 1;
    double lastLat = 0;
    double lastLon = 0;

    QSettings settings;
    settings.beginGroup("QGC_MAPWIDGET");
    if (changePosition)
    {
        lastLat = settings.value("LAST_LATITUDE", lastLat).toDouble();
        lastLon = settings.value("LAST_LONGITUDE", lastLon).toDouble();
        lastZoom = settings.value("LAST_ZOOM", lastZoom).toDouble();
    }
    trailType = static_cast<mapcontrol::UAVTrailType::Types>(settings.value("TRAIL_TYPE", trailType).toInt());
    trailInterval = settings.value("TRAIL_INTERVAL", trailInterval).toFloat();
    settings.endGroup();

#if 0
    // FIXME: NYI
    // SET CORRECT MENU CHECKBOXES
    // Set the correct trail interval
    if (trailType == mapcontrol::UAVTrailType::ByDistance)
    {
        // XXX
        qDebug() << "WARNING: Settings loading for trail type (ByDistance) not implemented";
    }
    else if (trailType == mapcontrol::UAVTrailType::ByTimeElapsed)
    {
        // XXX
        qDebug() << "WARNING: Settings loading for trail type (ByTimeElapsed) not implemented";
    }
#endif

    // SET TRAIL TYPE
    foreach (mapcontrol::UAVItem* uav, GetUAVS())
    {
        // Set the correct trail type
        uav->SetTrailType(trailType);
        // Set the correct trail interval
        if (trailType == mapcontrol::UAVTrailType::ByDistance)
        {
            uav->SetTrailDistance(trailInterval);
        }
        else if (trailType == mapcontrol::UAVTrailType::ByTimeElapsed)
        {
            uav->SetTrailTime(trailInterval);
        }
    }

    // SET INITIAL POSITION AND ZOOM
    internals::PointLatLng pos_lat_lon = internals::PointLatLng(lastLat, lastLon);
    SetCurrentPosition(pos_lat_lon);        // set the map position
    SetZoom(lastZoom); // set map zoom level
}

void QGCMapWidget::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAPWIDGET");
    internals::PointLatLng pos = CurrentPosition();
    settings.setValue("LAST_LATITUDE", pos.Lat());
    settings.setValue("LAST_LONGITUDE", pos.Lng());
    settings.setValue("LAST_ZOOM", ZoomReal());
    settings.setValue("TRAIL_TYPE", static_cast<int>(trailType));
    settings.setValue("TRAIL_INTERVAL", trailInterval);
    settings.endGroup();
}

void QGCMapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    // If a waypoint manager is available
    if (currWPManager)
    {
        // Create new waypoint
        internals::PointLatLng pos = map->FromLocalToLatLng(event->pos().x(), event->pos().y());
        Waypoint* wp = currWPManager->createWaypoint();
        wp->setLatitude(pos.Lat());
        wp->setLongitude(pos.Lng());
        wp->setFrame((MAV_FRAME)currWPManager->getFrameRecommendation());
        wp->setAltitude(currWPManager->getAltitudeRecommendation());
    }

    OPMapWidget::mouseDoubleClickEvent(event);
}


/**
 *
 * @param uas the UAS/MAV to monitor/display with the map widget
 */
void QGCMapWidget::addUAS(UASInterface* uas)
{
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,double,quint64)),
            this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,double,quint64)), Qt::UniqueConnection);
    connect(uas, SIGNAL(systemSpecsChanged(int)), this, SLOT(updateSystemSpecs(int)), Qt::UniqueConnection);
    if (!waypointLines.value(uas->getUASID(), NULL)) {
        waypointLines.insert(uas->getUASID(), new QGraphicsItemGroup(map));
    } else {
        foreach (QGraphicsItem* item, waypointLines.value(uas->getUASID())->childItems())
        {
            delete item;
        }
    }
}

void QGCMapWidget::activeUASSet(UASInterface* uas)
{
    // Only execute if proper UAS is set
    this->uas = uas;

    // Disconnect old MAV manager
    if (currWPManager)
    {
        // Disconnect the waypoint manager / data storage from the UI
        disconnect(currWPManager, SIGNAL(waypointEditableListChanged(int)), this, SLOT(updateWaypointList(int)));
        disconnect(currWPManager, SIGNAL(waypointEditableChanged(int, Waypoint*)), this, SLOT(updateWaypoint(int,Waypoint*)));
        disconnect(this, SIGNAL(waypointCreated(Waypoint*)), currWPManager, SLOT(addWaypointEditable(Waypoint*)));
        disconnect(this, SIGNAL(waypointChanged(Waypoint*)), currWPManager, SLOT(notifyOfChangeEditable(Waypoint*)));
    }

    // Attach the new waypoint manager if a new UAS was selected. Otherwise, indicate
    // that no such manager exists.
    if (uas)
    {
        currWPManager = uas->getWaypointManager();

        updateSelectedSystem(uas->getUASID());
        followUAVID = uas->getUASID();
        updateWaypointList(uas->getUASID());

        // Connect the waypoint manager / data storage to the UI
        connect(currWPManager, SIGNAL(waypointEditableListChanged(int)), this, SLOT(updateWaypointList(int)), Qt::UniqueConnection);
        connect(currWPManager, SIGNAL(waypointEditableChanged(int, Waypoint*)), this, SLOT(updateWaypoint(int,Waypoint*)), Qt::UniqueConnection);
        connect(this, SIGNAL(waypointCreated(Waypoint*)), currWPManager, SLOT(addWaypointEditable(Waypoint*)), Qt::UniqueConnection);
        connect(this, SIGNAL(waypointChanged(Waypoint*)), currWPManager, SLOT(notifyOfChangeEditable(Waypoint*)), Qt::UniqueConnection);

        if (!mapPositionInitialized) {
            internals::PointLatLng pos_lat_lon = internals::PointLatLng(uas->getLatitude(), uas->getLongitude());
            SetCurrentPosition(pos_lat_lon);

            // Zoom in
            SetZoom(13);

            mapPositionInitialized = true;
        }
    }
    else
    {
        currWPManager = NULL;
    }
}

/**
 * Updates the global position of one MAV and append the last movement to the trail
 *
 * @param uas The unmanned air system
 * @param lat Latitude in WGS84 ellipsoid
 * @param lon Longitutde in WGS84 ellipsoid
 * @param alt Altitude over mean sea level
 * @param usec Timestamp of the position message in milliseconds FIXME will move to microseconds
 */
void QGCMapWidget::updateGlobalPosition(UASInterface* uas, double lat, double lon, double altAMSL, double altWGS84, quint64 usec)
{
    Q_UNUSED(usec);
    Q_UNUSED(altAMSL);

    // Immediate update
    if (maxUpdateInterval == 0)
    {
        // Get reference to graphic UAV item
        mapcontrol::UAVItem* uav = GetUAV(uas->getUASID());
        // Check if reference is valid, else create a new one
        if (uav == NULL)
        {
            MAV2DIcon* newUAV = new MAV2DIcon(map, this, uas);
            newUAV->setParentItem(map);
            UAVS.insert(uas->getUASID(), newUAV);
            uav = GetUAV(uas->getUASID());
            // Set the correct trail type
            uav->SetTrailType(trailType);
            // Set the correct trail interval
            if (trailType == mapcontrol::UAVTrailType::ByDistance)
            {
                uav->SetTrailDistance(trailInterval);
            }
            else if (trailType == mapcontrol::UAVTrailType::ByTimeElapsed)
            {
                uav->SetTrailTime(trailInterval);
            }
        }

        // Set new lat/lon position of UAV icon
        internals::PointLatLng pos_lat_lon = internals::PointLatLng(lat, lon);
        uav->SetUAVPos(pos_lat_lon, altWGS84);
        // Follow status
        if (followUAVEnabled && uas->getUASID() == followUAVID) SetCurrentPosition(pos_lat_lon);
        // Convert from radians to degrees and apply
        uav->SetUAVHeading((uas->getYaw()/M_PI)*180.0f);
    }
}

/**
 * Pulls in the positions of all UAVs from the UAS manager
 */
void QGCMapWidget::updateGlobalPosition()
{
    QList<UASInterface*> systems = UASManager::instance()->getUASList();
    foreach (UASInterface* system, systems)
    {
        // Get reference to graphic UAV item
        mapcontrol::UAVItem* uav = GetUAV(system->getUASID());
        // Check if reference is valid, else create a new one
        if (uav == NULL)
        {
            MAV2DIcon* newUAV = new MAV2DIcon(map, this, system);
            AddUAV(system->getUASID(), newUAV);
            uav = newUAV;
            uav->SetTrailTime(1);
            uav->SetTrailDistance(5);
            uav->SetTrailType(mapcontrol::UAVTrailType::ByTimeElapsed);
        }

        // Set new lat/lon position of UAV icon
        internals::PointLatLng pos_lat_lon = internals::PointLatLng(system->getLatitude(), system->getLongitude());
        uav->SetUAVPos(pos_lat_lon, system->getAltitudeAMSL());
        // Follow status
        if (followUAVEnabled && system->getUASID() == followUAVID) SetCurrentPosition(pos_lat_lon);
        // Convert from radians to degrees and apply
        uav->SetUAVHeading((system->getYaw()/M_PI)*180.0f);
    }
}

void QGCMapWidget::updateLocalPosition()
{
    QList<UASInterface*> systems = UASManager::instance()->getUASList();
    foreach (UASInterface* system, systems)
    {
        // Get reference to graphic UAV item
        mapcontrol::UAVItem* uav = GetUAV(system->getUASID());
        // Check if reference is valid, else create a new one
        if (uav == NULL)
        {
            MAV2DIcon* newUAV = new MAV2DIcon(map, this, system);
            AddUAV(system->getUASID(), newUAV);
            uav = newUAV;
            uav->SetTrailTime(1);
            uav->SetTrailDistance(5);
            uav->SetTrailType(mapcontrol::UAVTrailType::ByTimeElapsed);
        }

        // Set new lat/lon position of UAV icon
        internals::PointLatLng pos_lat_lon = internals::PointLatLng(system->getLatitude(), system->getLongitude());
        uav->SetUAVPos(pos_lat_lon, system->getAltitudeAMSL());
        // Follow status
        if (followUAVEnabled && system->getUASID() == followUAVID) SetCurrentPosition(pos_lat_lon);
        // Convert from radians to degrees and apply
        uav->SetUAVHeading((system->getYaw()/M_PI)*180.0f);
    }
}

void QGCMapWidget::updateLocalPositionEstimates()
{
    updateLocalPosition();
}


void QGCMapWidget::updateSystemSpecs(int uas)
{
    foreach (mapcontrol::UAVItem* p, UAVS.values())
    {
        MAV2DIcon* icon = dynamic_cast<MAV2DIcon*>(p);
        if (icon && icon->getUASId() == uas)
        {
            // Set new airframe
            icon->setAirframe(UASManager::instance()->getUASForId(uas)->getAirframe());
            icon->drawIcon();
        }
    }
}

/**
 * Does not update the system type or configuration, only the selected status
 */
void QGCMapWidget::updateSelectedSystem(int uas)
{
    foreach (mapcontrol::UAVItem* p, UAVS.values())
    {
        MAV2DIcon* icon = dynamic_cast<MAV2DIcon*>(p);
        if (icon)
        {
            // Set as selected if ids match
            icon->setSelectedUAS((icon->getUASId() == uas));
        }
    }
}


// MAP NAVIGATION
void QGCMapWidget::showGoToDialog()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Please enter coordinates"),
                                         tr("Coordinates (Lat,Lon):"), QLineEdit::Normal,
                                         QString("%1,%2").arg(CurrentPosition().Lat(), 0, 'g', 6).arg(CurrentPosition().Lng(), 0, 'g', 6), &ok);
    if (ok && !text.isEmpty())
    {
        QStringList split = text.split(",");
        if (split.length() == 2)
        {
            bool convert;
            double latitude = split.first().toDouble(&convert);
            ok &= convert;
            double longitude = split.last().toDouble(&convert);
            ok &= convert;

            if (ok)
            {
                internals::PointLatLng pos_lat_lon = internals::PointLatLng(latitude, longitude);
                SetCurrentPosition(pos_lat_lon);        // set the map position
            }
        }
    }
}


void QGCMapWidget::updateHomePosition(double latitude, double longitude, double altitude)
{
    qDebug() << "HOME SET TO: " << latitude << longitude << altitude;
    Home->SetCoord(internals::PointLatLng(latitude, longitude));
    Home->SetAltitude(altitude);
    homeAltitude = altitude;
    SetShowHome(true);                      // display the HOME position on the map
    Home->RefreshPos();
}

void QGCMapWidget::goHome()
{
    SetCurrentPosition(Home->Coord());
    SetZoom(17);
}

/**
 * Limits the update rate on the specified interval. Set to zero (0) to run at maximum
 * telemetry speed. Recommended rate is 2 s.
 */
void QGCMapWidget::setUpdateRateLimit(float seconds)
{
    maxUpdateInterval = seconds;
    updateTimer.start(maxUpdateInterval*1000);
}

void QGCMapWidget::cacheVisibleRegion()
{
    internals::RectLatLng rect = map->SelectedArea();

    if (rect.IsEmpty())
    {
        QGCMessageBox::information(tr("Cannot cache tiles for offline use"),
                                   tr("Please select an area first by holding down SHIFT or ALT and selecting the area with the left mouse button."));
    }
    else
    {
        RipMap();
        // Set empty area = unselect area
        map->SetSelectedArea(internals::RectLatLng());
    }
}


// WAYPOINT MAP INTERACTION FUNCTIONS

void QGCMapWidget::handleMapWaypointEdit(mapcontrol::WayPointItem* waypoint)
{
    // Block circle updates
    Waypoint* wp = iconsToWaypoints.value(waypoint, NULL);

    // Delete UI element if wp doesn't exist
    if (!wp)
        WPDelete(waypoint);

    // Update WP values
    internals::PointLatLng pos = waypoint->Coord();

    // Block waypoint signals
    wp->blockSignals(true);
    wp->setLatitude(pos.Lat());
    wp->setLongitude(pos.Lng());
    wp->blockSignals(false);


//    internals::PointLatLng coord = waypoint->Coord();
//    QString coord_str = " " + QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);
//    qDebug() << "MAP WP COORD (MAP):" << coord_str << __FILE__ << __LINE__;
//    QString wp_str = QString::number(wp->getLatitude(), 'f', 6) + "   " + QString::number(wp->getLongitude(), 'f', 6);
//    qDebug() << "MAP WP COORD (WP):" << wp_str << __FILE__ << __LINE__;

    // Protect from vicious double update cycle
    if (firingWaypointChange == wp) {
        return;
    }
    // Not in cycle, block now from entering it
    firingWaypointChange = wp;

    emit waypointChanged(wp);
}

// WAYPOINT UPDATE FUNCTIONS

/**
 * This function is called if a a single waypoint is updated and
 * also if the whole list changes.
 */
void QGCMapWidget::updateWaypoint(int uas, Waypoint* wp)
{
    //qDebug() << __FILE__ << __LINE__ << "UPDATING WP FUNCTION CALLED";
    // Source of the event was in this widget, do nothing
    if (firingWaypointChange == wp) {
        return;
    }
    // Currently only accept waypoint updates from the UAS in focus
    // this has to be changed to accept read-only updates from other systems as well.
    UASInterface* uasInstance = UASManager::instance()->getUASForId(uas);
    if (currWPManager)
    {
        // Only accept waypoints in global coordinate frame
        if (((wp->getFrame() == MAV_FRAME_GLOBAL) || (wp->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT)) && wp->isNavigationType())
        {
            // We're good, this is a global waypoint

            // Get the index of this waypoint
            // note the call to getGlobalFrameAndNavTypeIndexOf()
            // as we're only handling global waypoints
            int wpindex = currWPManager->getGlobalFrameAndNavTypeIndexOf(wp);
            // If not found, return (this should never happen, but helps safety)
            if (wpindex < 0) return;
            // Mark this wp as currently edited
            firingWaypointChange = wp;

            qDebug() << "UPDATING WAYPOINT" << wpindex << "IN 2D MAP";

            // Check if wp exists yet in map
            if (!waypointsToIcons.contains(wp))
            {
                // Create icon for new WP
                QColor wpColor(Qt::red);
                if (uasInstance) wpColor = uasInstance->getColor();
                Waypoint2DIcon* icon = new Waypoint2DIcon(map, this, wp, wpColor, wpindex);
                ConnectWP(icon);
                icon->setParentItem(map);
                // Update maps to allow inverse data association
                waypointsToIcons.insert(wp, icon);
                iconsToWaypoints.insert(icon, wp);

                // Add line element if this is NOT the first waypoint
                if (wpindex > 0)
                {
                    // Get predecessor of this WP
                    QList<Waypoint* > wps = currWPManager->getGlobalFrameAndNavTypeWaypointList();
                    Waypoint* wp1 = wps.at(wpindex-1);
                    mapcontrol::WayPointItem* prevIcon = waypointsToIcons.value(wp1, NULL);
                    // If we got a valid graphics item, continue
                    if (prevIcon)
                    {
                        mapcontrol::WaypointLineItem* line = new mapcontrol::WaypointLineItem(prevIcon, icon, wpColor, map);
                        line->setParentItem(map);
                        QGraphicsItemGroup* group = waypointLines.value(uas, NULL);
                        if (group)
                        {
                            group->addToGroup(line);
                            group->setParentItem(map);
                        }
                    }
                }
            }
            else
            {
                // Waypoint exists, block it's signals and update it
                mapcontrol::WayPointItem* icon = waypointsToIcons.value(wp);
                // Make sure we don't die on a null pointer
                // this should never happen, just a precaution
                if (!icon) return;
                // Block outgoing signals to prevent an infinite signal loop
                // should not happen, just a precaution
                this->blockSignals(true);
                // Update the WP
                Waypoint2DIcon* wpicon = dynamic_cast<Waypoint2DIcon*>(icon);
                if (wpicon)
                {
                    // Let icon read out values directly from waypoint
                    icon->SetNumber(wpindex);
                    wpicon->updateWaypoint();
                }
                else
                {
                    // Use safe standard interfaces for non Waypoint-class based wps
                    icon->SetCoord(internals::PointLatLng(wp->getLatitude(), wp->getLongitude()));
                    icon->SetAltitude(wp->getAltitude());
                    icon->SetHeading(wp->getYaw());
                    icon->SetNumber(wpindex);
                }
                // Re-enable signals again
                this->blockSignals(false);
            }

            firingWaypointChange = NULL;

        }
        else
        {
            // Check if the index of this waypoint is larger than the global
            // waypoint list. This implies that the coordinate frame of this
            // waypoint was changed and the list containing only global
            // waypoints was shortened. Thus update the whole list
            if (waypointsToIcons.count() > currWPManager->getGlobalFrameAndNavTypeCount())
            {
                updateWaypointList(uas);
            }
        }
    }
}

/**
 * Update the whole list of waypoints. This is e.g. necessary if the list order changed.
 * The UAS manager will emit the appropriate signal whenever updating the list
 * is necessary.
 */
void QGCMapWidget::updateWaypointList(int uas)
{
    qDebug() << "UPDATE WP LIST IN 2D MAP CALLED FOR UAS" << uas;
    // Currently only accept waypoint updates from the UAS in focus
    // this has to be changed to accept read-only updates from other systems as well.
    UASInterface* uasInstance = UASManager::instance()->getUASForId(uas);
    if (currWPManager)
    {
        // ORDER MATTERS HERE!
        // TWO LOOPS ARE NEEDED - INFINITY LOOP ELSE

        qDebug() << "DELETING NOW OLD WPS";

        // Delete connecting waypoint lines
        QGraphicsItemGroup* group = waypointLines.value(uas, NULL);
        if (group)
        {
            foreach (QGraphicsItem* item, group->childItems())
            {
                delete item;
            }
        }

        // Delete first all old waypoints
        // this is suboptimal (quadratic, but wps should stay in the sub-100 range anyway)
        QList<Waypoint* > wps = currWPManager->getGlobalFrameAndNavTypeWaypointList();
        foreach (Waypoint* wp, waypointsToIcons.keys())
        {
            if (!wps.contains(wp))
            {
                // Get icon to work on
                mapcontrol::WayPointItem* icon = waypointsToIcons.value(wp);
                waypointsToIcons.remove(wp);
                iconsToWaypoints.remove(icon);
                WPDelete(icon);
            }
        }

        // Update all existing waypoints
        foreach (Waypoint* wp, waypointsToIcons.keys())
        {
            // Update remaining waypoints
            updateWaypoint(uas, wp);
        }

        // Update all potentially new waypoints
        foreach (Waypoint* wp, wps)
        {
            qDebug() << "UPDATING NEW WP" << wp->getId();
            // Update / add only if new
            if (!waypointsToIcons.contains(wp)) updateWaypoint(uas, wp);
        }

        // Add line element if this is NOT the first waypoint
        mapcontrol::WayPointItem* prevIcon = NULL;
        foreach (Waypoint* wp, wps)
        {
            mapcontrol::WayPointItem* currIcon = waypointsToIcons.value(wp, NULL);
            // Do not work on first waypoint, but only increment counter
            // do not continue if icon is invalid
            if (prevIcon && currIcon)
            {
                // If we got a valid graphics item, continue
                QColor wpColor(Qt::red);
                if (uasInstance) wpColor = uasInstance->getColor();
                mapcontrol::WaypointLineItem* line = new mapcontrol::WaypointLineItem(prevIcon, currIcon, wpColor, map);
                line->setParentItem(map);
                QGraphicsItemGroup* group = waypointLines.value(uas, NULL);
                if (group)
                {
                    group->addToGroup(line);
                    group->setParentItem(map);
                }
            }
            prevIcon = currIcon;
        }
    }
}


//// ADAPTER / HELPER FUNCTIONS
//float QGCMapWidget::metersToPixels(double meters)
//{
//    return meters/map->Projection()->GetGroundResolution(map->ZoomTotal(),coord.Lat());
//}

//double QGCMapWidget::headingP1P2(internals::PointLatLng p1, internals::PointLatLng p2)
//{
//    double lat1 = p1.Lat() * deg_to_rad;
//    double lon1 = p2.Lng() * deg_to_rad;

//    double lat2 = p2.Lat() * deg_to_rad;
//    double lon2 = p2.Lng() * deg_to_rad;

//    double delta_lon = lon2 - lon1;

//    double y = sin(delta_lon) * cos(lat2);
//    double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(delta_lon);
//    double heading = atan2(y, x) * rad_to_deg;

//    heading += 360;
//    while (heading < 0) bear += 360;
//    while (heading >= 360) bear -= 360;

//    return heading;
//}

//internals::PointLatLng QGCMapWidget::targetLatLon(internals::PointLatLng source, double heading, double dist)
//{
//    double lat1 = source.Lat() * deg_to_rad;
//    double lon1 = source.Lng() * deg_to_rad;

//    heading *= deg_to_rad;

//    double ad = dist / earth_mean_radius;

//    double lat2 = asin(sin(lat1) * cos(ad) + cos(lat1) * sin(ad) * cos(heading));
//    double lon2 = lon1 + atan2(sin(bear) * sin(ad) * cos(lat1), cos(ad) - sin(lat1) * sin(lat2));

//    return internals::PointLatLng(lat2 * rad_to_deg, lon2 * rad_to_deg);
//}
