#include "QGCMapWidget.h"
#include "UASInterface.h"
#include "UASManager.h"

QGCMapWidget::QGCMapWidget(QWidget *parent) :
        mapcontrol::OPMapWidget(parent)
{
    //UAV = new mapcontrol::UAVItem();
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
    foreach (UASInterface* uas, UASManager::instance()->getUASList())
    {
        addUAS(uas);
    }
    UAV->setVisible(true);
    UAV->setPos(0, 0);
    UAV->show();




    internals::PointLatLng pos_lat_lon = internals::PointLatLng(0, 0);

    //        // **************
    //        // default home position

    //        home_position.coord = pos_lat_lon;
    //        home_position.altitude = altitude;
    //        home_position.locked = false;

    //        // **************
    //        // default magic waypoint params

    //        magic_waypoint.map_wp_item = NULL;
    //        magic_waypoint.coord = home_position.coord;
    //        magic_waypoint.altitude = altitude;
    //        magic_waypoint.description = "Magic waypoint";
    //        magic_waypoint.locked = false;
    //        magic_waypoint.time_seconds = 0;
    //        magic_waypoint.hold_time_seconds = 0;

    const int safe_area_radius_list[] = {5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};   // meters

    const int uav_trail_time_list[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};                      // seconds

    const int uav_trail_distance_list[] = {1, 2, 5, 10, 20, 50, 100, 200, 500};             // meters

    SetMouseWheelZoomType(internals::MouseWheelZoomType::MousePositionWithoutCenter);	    // set how the mouse wheel zoom functions
    SetFollowMouse(true);				    // we want a contiuous mouse position reading

    SetShowHome(true);					    // display the HOME position on the map
    SetShowUAV(true);					    // display the UAV position on the map

    Home->SetSafeArea(safe_area_radius_list[0]);                         // set radius (meters)
    Home->SetShowSafeArea(true);                                         // show the safe area

    UAV->SetTrailTime(uav_trail_time_list[0]);                           // seconds
    UAV->SetTrailDistance(uav_trail_distance_list[1]);                   // meters

    //UAV->SetTrailType(UAVTrailType::ByTimeElapsed);
    //  UAV->SetTrailType(UAVTrailType::ByDistance);

    GPS->SetTrailTime(uav_trail_time_list[0]);                           // seconds
    GPS->SetTrailDistance(uav_trail_distance_list[1]);                   // meters

    //GPS->SetTrailType(UAVTrailType::ByTimeElapsed);

    SetCurrentPosition(pos_lat_lon);         // set the map position
    Home->SetCoord(pos_lat_lon);             // set the HOME position
    UAV->SetUAVPos(pos_lat_lon, 0.0);        // set the UAV position
    GPS->SetUAVPos(pos_lat_lon, 0.0);        // set the UAV position

    //SetUAVPos(pos_lat_lon, 0.0);        // set the UAV position

    setFrameStyle(QFrame::NoFrame);      // no border frame
    setBackgroundBrush(QBrush(Qt::black)); // tile background

    setFocus();
}

QGCMapWidget::~QGCMapWidget()
{
    SetShowHome(false);	// doing this appears to stop the map lib crashing on exit
    SetShowUAV(false);	//   "          "
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void QGCMapWidget::addUAS(UASInterface* uas)
{
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    //connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));
    //connect(uas, SIGNAL(systemSpecsChanged(int)), this, SLOT(updateSystemSpecs(int)));
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
void QGCMapWidget::updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec)
{
    Q_UNUSED(usec);
    Q_UNUSED(alt); // FIXME Use altitude

    UAV->setPos(lat, lon);
    UAV->show();
    qDebug() << "Updating 2D map position";

//        QPointF coordinate;
//        coordinate.setX(lon);
//        coordinate.setY(lat);



//        if (!uasIcons.contains(uas->getUASID())) {
//            // Get the UAS color
//            QColor uasColor = uas->getColor();

//            // Icon
//            //QPen* pointpen = new QPen(uasColor);
//            qDebug() << "2D MAP: ADDING" << uas->getUASName() << __FILE__ << __LINE__;
//            p = new MAV2DIcon(uas, 68, uas->getSystemType(), uas->getColor(), QString("%1").arg(uas->getUASID()), qmapcontrol::Point::Middle);
//            uasIcons.insert(uas->getUASID(), p);
//            mc->layer("Waypoints")->addGeometry(p);

//            // Line
//            // A QPen also can use transparency

//            //        QList<qmapcontrol::Point*> points;
//            //        points.append(new qmapcontrol::Point(coordinate.x(), coordinate.y()));
//            //        QPen* linepen = new QPen(uasColor.darker());
//            //        linepen->setWidth(2);

//            //        // Create tracking line string
//            //        qmapcontrol::LineString* ls = new qmapcontrol::LineString(points, QString("%1").arg(uas->getUASID()), linepen);
//            //        uasTrails.insert(uas->getUASID(), ls);

//            //        // Add the LineString to the layer
//            //        mc->layer("Waypoints")->addGeometry(ls);
//        } else {
//            //        p = dynamic_cast<MAV2DIcon*>(uasIcons.value(uas->getUASID()));
//            //        if (p)
//            //        {
//            p = uasIcons.value(uas->getUASID());
//            p->setCoordinate(QPointF(lon, lat));
//            //p->setYaw(uas->getYaw());
//            //        }
//            // Extend trail
//            //        uasTrails.value(uas->getUASID())->addPoint(new qmapcontrol::Point(coordinate.x(), coordinate.y()));
//        }

//        if (isVisible()) mc->updateRequest(p->boundingBox().toRect());

//        //if (isVisible()) mc->updateRequestNew();//(uasTrails.value(uas->getUASID())->boundingBox().toRect());

//        if (this->mav && uas->getUASID() == this->mav->getUASID()) {
//            // Limit the position update rate
//            quint64 currTime = MG::TIME::getGroundTimeNow();
//            if (currTime - lastUpdate > 120) {
//                lastUpdate = currTime;
//                // Sets the view to the interesting area
//                if (followgps->isChecked()) {
//                    updatePosition(0, lon, lat);
//                } else {
//                    // Refresh the screen
//                    //if (isVisible()) mc->updateRequestNew();
//                }
//            }
//        }
}
