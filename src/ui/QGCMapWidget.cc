#include "QGCMapWidget.h"
#include "UASInterface.h"

QGCMapWidget::QGCMapWidget(QWidget *parent) :
    mapcontrol::OPMapWidget(parent)
{
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
