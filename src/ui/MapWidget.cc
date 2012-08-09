/*==================================================================
======================================================================*/

/**
 * @file
 *   @brief Implementation of MapWidget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Mariano Lizarraga
 *
 */

#include <QComboBox>
#include <QGridLayout>
#include <QDir>

#include "QGC.h"
#include "MapWidget.h"
#include "ui_MapWidget.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "MAV2DIcon.h"
#include "Waypoint2DIcon.h"
#include "UASWaypointManager.h"

#include "MG.h"


MapWidget::MapWidget(QWidget *parent) :
    QWidget(parent),
    mc(NULL),
    zoomLevel(0),
    uasIcons(),
    uasTrails(),
    mav(NULL),
    lastUpdate(0),
    initialized(false),
    m_ui(new Ui::MapWidget)
{
    m_ui->setupUi(this);
    init();
}

void MapWidget::init()
{
    if (!initialized) {
        mc = new qmapcontrol::MapControl(this->size());
        // display the MapControl in the application
        QGridLayout* layout = new QGridLayout(this);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(mc, 0, 0);
        setLayout(layout);

        //   VISUAL MAP STYLE
        QString buttonStyle("QAbstractButton { background-color: rgba(20, 20, 20, 45%); border-color: rgba(10, 10, 10, 50%)} QAbstractButton:checked { border: 2px solid #379AC3; }");
        mc->setPen(QGC::colorCyan.darker(400));

        waypointIsDrag = false;

        // Accept focus by clicking or keyboard
        this->setFocusPolicy(Qt::StrongFocus);

        // create MapControl

        mc->showScale(true);
        mc->showCoord(true);
        mc->enablePersistentCache();
        mc->setMouseTracking(true); // required to update the mouse position for diplay and capture

        // create MapAdapter to get maps from
        //TileMapAdapter* osmAdapter = new TileMapAdapter("tile.openstreetmap.org", "/%1/%2/%3.png", 256, 0, 17);

        qmapcontrol::MapAdapter* mapadapter_overlay = new qmapcontrol::YahooMapAdapter("us.maps3.yimg.com", "/aerial.maps.yimg.com/png?v=2.2&t=h&s=256&x=%2&y=%3&z=%1");

        // MAP BACKGROUND
        mapadapter = new qmapcontrol::GoogleSatMapAdapter();
        l = new qmapcontrol::MapLayer("Google Satellite", mapadapter);
        mc->addLayer(l);

        // STREET OVERLAY
        overlay = new qmapcontrol::MapLayer("Overlay", mapadapter_overlay);
        overlay->setVisible(false);
        mc->addLayer(overlay);

        // MAV FLIGHT TRACKS
        tracks = new qmapcontrol::MapLayer("Tracking", mapadapter);
        mc->addLayer(tracks);

        // WAYPOINT LAYER
        // create a layer with the mapadapter and type GeometryLayer (for waypoints)
        geomLayer = new qmapcontrol::GeometryLayer("Waypoints", mapadapter);
        mc->addLayer(geomLayer);



        //
        //    Layer* gsatLayer = new Layer("Google Satellite", gsat, Layer::MapLayer);
        //    mc->addLayer(gsatLayer);


        // Zurich, ETH

        int lastZoom = 16;
        double lastLat = 47.376889;
        double lastLon = 8.548056;

        QSettings settings;
        settings.beginGroup("QGC_MAPWIDGET");
        lastLat = settings.value("LAST_LATITUDE", lastLat).toDouble();
        lastLon = settings.value("LAST_LONGITUDE", lastLon).toDouble();
        lastZoom = settings.value("LAST_ZOOM", lastZoom).toInt();
        settings.endGroup();

        // SET INITIAL POSITION AND ZOOM
        // Set default zoom level
        mc->setZoom(lastZoom);
        mc->setView(QPointF(lastLon, lastLat));

        // Veracruz Mexico
        //mc->setView(QPointF(-96.105208,19.138955));

        // Add controls to select map provider
        /////////////////////////////////////////////////
        QActionGroup* mapproviderGroup = new QActionGroup(this);
        osmAction = new QAction(QIcon(":/files/images/mapproviders/openstreetmap.png"), tr("OpenStreetMap"), mapproviderGroup);
        yahooActionMap = new QAction(QIcon(":/files/images/mapproviders/yahoo.png"), tr("Yahoo: Map"), mapproviderGroup);
        yahooActionSatellite = new QAction(QIcon(":/files/images/mapproviders/yahoo.png"), tr("Yahoo: Satellite"), mapproviderGroup);
        googleActionMap = new QAction(QIcon(":/files/images/mapproviders/google.png"), tr("Google: Map"), mapproviderGroup);
        googleSatAction = new QAction(QIcon(":/files/images/mapproviders/google.png"), tr("Google: Sat"), mapproviderGroup);
        osmAction->setCheckable(true);
        yahooActionMap->setCheckable(true);
        yahooActionSatellite->setCheckable(true);
        googleActionMap->setCheckable(true);
        googleSatAction->setCheckable(true);
        googleSatAction->setChecked(true);
        connect(mapproviderGroup, SIGNAL(triggered(QAction*)),
                this, SLOT(mapproviderSelected(QAction*)));

        // Overlay seems currently broken
        //    yahooActionOverlay = new QAction(tr("Yahoo: street overlay"), this);
        //    yahooActionOverlay->setCheckable(true);
        //    yahooActionOverlay->setChecked(overlay->isVisible());
        //    connect(yahooActionOverlay, SIGNAL(toggled(bool)),
        //            overlay, SLOT(setVisible(bool)));

        //    mapproviderGroup->addAction(googleSatAction);
        //    mapproviderGroup->addAction(osmAction);
        //    mapproviderGroup->addAction(yahooActionOverlay);
        //    mapproviderGroup->addAction(googleActionMap);
        //    mapproviderGroup->addAction(yahooActionMap);
        //    mapproviderGroup->addAction(yahooActionSatellite);

        // Create map provider selection menu
        mapMenu = new QMenu(this);
        mapMenu->addActions(mapproviderGroup->actions());
        mapMenu->addSeparator();
        //    mapMenu->addAction(yahooActionOverlay);

        mapButton = new QPushButton(this);
        mapButton->setText("Map Source");
        mapButton->setMenu(mapMenu);
        mapButton->setStyleSheet(buttonStyle);

        // create buttons to control the map (zoom, GPS tracking and WP capture)
        QPushButton* zoomin = new QPushButton(QIcon(":/files/images/actions/list-add.svg"), "", this);
        zoomin->setStyleSheet(buttonStyle);
        QPushButton* zoomout = new QPushButton(QIcon(":/files/images/actions/list-remove.svg"), "", this);
        zoomout->setStyleSheet(buttonStyle);
        createPath = new QPushButton(QIcon(":/files/images/actions/go-bottom.svg"), "", this);
        createPath->setStyleSheet(buttonStyle);
        createPath->setToolTip(tr("Start / end waypoint add mode"));
        createPath->setStatusTip(tr("Start / end waypoint add mode"));
        //    clearTracking = new QPushButton(QIcon(""), "", this);
        //    clearTracking->setStyleSheet(buttonStyle);
        followgps = new QPushButton(QIcon(":/files/images/actions/system-lock-screen.svg"), "", this);
        followgps->setStyleSheet(buttonStyle);
        followgps->setToolTip(tr("Follow the position of the current MAV with the map center"));
        followgps->setStatusTip(tr("Follow the position of the current MAV with the map center"));
        QPushButton* goToButton = new QPushButton(QIcon(""), "T", this);
        goToButton->setStyleSheet(buttonStyle);
        goToButton->setToolTip(tr("Enter a latitude/longitude position to move the map to"));
        goToButton->setStatusTip(tr("Enter a latitude/longitude position to move the map to"));

        zoomin->setMaximumWidth(30);
        zoomout->setMaximumWidth(30);
        createPath->setMaximumWidth(30);
        //    clearTracking->setMaximumWidth(30);
        followgps->setMaximumWidth(30);
        goToButton->setMaximumWidth(30);

        // Set checkable buttons
        // TODO: Currently checked buttons are are very difficult to distinguish when checked.
        //       create a style and the slots to change the background so it is easier to distinguish
        followgps->setCheckable(true);
        createPath->setCheckable(true);

        // add buttons to control the map (zoom, GPS tracking and WP capture)
        QGridLayout* innerlayout = new QGridLayout(mc);
        innerlayout->setMargin(3);
        innerlayout->setSpacing(3);
        innerlayout->addWidget(zoomin, 0, 0);
        innerlayout->addWidget(zoomout, 1, 0);
        innerlayout->addWidget(followgps, 2, 0);
        innerlayout->addWidget(createPath, 3, 0);
        //innerlayout->addWidget(clearTracking, 4, 0);
        // Add spacers to compress buttons on the top left
        innerlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 5, 0);
        innerlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, 1, 0, 7);
        innerlayout->addWidget(mapButton, 0, 6);
        innerlayout->addWidget(goToButton, 0, 7);
        innerlayout->setRowStretch(0, 1);
        innerlayout->setRowStretch(1, 100);
        mc->setLayout(innerlayout);

        // Configure the WP Path's pen
        pointPen = new QPen(QColor(0, 255,0));
        pointPen->setWidth(3);
        waypointPath = new qmapcontrol::LineString (wps, "Waypoint path", pointPen);
        mc->layer("Waypoints")->addGeometry(waypointPath);

        //Camera Control
        // CAMERA INDICATOR LAYER
        // create a layer with the mapadapter and type GeometryLayer (for camera indicator)
        camLayer = new qmapcontrol::GeometryLayer("Camera", mapadapter);
        mc->addLayer(camLayer);

        //camLine = new qmapcontrol::LineString(camPoints,"Camera Eje", camBorderPen);

        drawCamBorder = false;
        radioCamera = 10;

        // Done set state
        initialized = true;


        // Connect the required signals-slots
        connect(zoomin, SIGNAL(clicked(bool)),
                mc, SLOT(zoomIn()));

        connect(zoomout, SIGNAL(clicked(bool)),
                mc, SLOT(zoomOut()));

        connect(goToButton, SIGNAL(clicked()), this, SLOT(goTo()));

        QList<UASInterface*> systems = UASManager::instance()->getUASList();
        foreach(UASInterface* system, systems) {
            addUAS(system);
        }

        connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
                this, SLOT(addUAS(UASInterface*)));

        activeUASSet(UASManager::instance()->getActiveUAS());
        connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(activeUASSet(UASInterface*)));

        connect(mc, SIGNAL(mouseEventCoordinate(const QMouseEvent*, const QPointF)),
                this, SLOT(captureMapClick(const QMouseEvent*, const QPointF)));

        connect(createPath, SIGNAL(clicked(bool)),
                this, SLOT(createPathButtonClicked(bool)));


        connect(geomLayer, SIGNAL(geometryClicked(Geometry*,QPoint)),
                this, SLOT(captureGeometryClick(Geometry*, QPoint)));

        connect(geomLayer, SIGNAL(geometryDragged(Geometry*, QPointF)),
                this, SLOT(captureGeometryDrag(Geometry*, QPointF)));

        connect(geomLayer, SIGNAL(geometryEndDrag(Geometry*, QPointF)),
                this, SLOT(captureGeometryEndDrag(Geometry*, QPointF)));

        qDebug() << "CHECK END";
    }
}

void MapWidget::goTo()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Please enter coordinates"),
                                         tr("Coordinates (Lat,Lon):"), QLineEdit::Normal,
                                         QString("%1,%2").arg(mc->currentCoordinate().y()).arg(mc->currentCoordinate().x()), &ok);
    if (ok && !text.isEmpty()) {
        QStringList split = text.split(",");
        if (split.length() == 2) {
            bool convert;
            double latitude = split.first().toDouble(&convert);
            ok &= convert;
            double longitude = split.last().toDouble(&convert);
            ok &= convert;

            if (ok) {
                mc->setView(QPointF(longitude, latitude));
            }
        }
    }
}


void MapWidget::mapproviderSelected(QAction* action)
{
    if (mc) {
        //delete mapadapter;
        mapButton->setText(action->text());
        if (action == osmAction) {
            int zoom = mapadapter->adaptedZoom();
            mc->setZoom(0);

            mapadapter = new qmapcontrol::OSMMapAdapter();
            l->setMapAdapter(mapadapter);
            geomLayer->setMapAdapter(mapadapter);

            if (isVisible()) mc->updateRequestNew();
            mc->setZoom(zoom);
            //        yahooActionOverlay->setEnabled(false);
            overlay->setVisible(false);
            //        yahooActionOverlay->setChecked(false);

        } else if (action == yahooActionMap) {
            int zoom = mapadapter->adaptedZoom();
            mc->setZoom(0);

            mapadapter = new qmapcontrol::YahooMapAdapter();
            l->setMapAdapter(mapadapter);
            geomLayer->setMapAdapter(mapadapter);

            if (isVisible()) mc->updateRequestNew();
            mc->setZoom(zoom);
            //        yahooActionOverlay->setEnabled(false);
            overlay->setVisible(false);
            //        yahooActionOverlay->setChecked(false);
        } else if (action == yahooActionSatellite) {
            int zoom = mapadapter->adaptedZoom();
            QPointF a = mc->currentCoordinate();
            mc->setZoom(0);

            mapadapter = new qmapcontrol::YahooMapAdapter("us.maps3.yimg.com", "/aerial.maps.yimg.com/png?v=1.7&t=a&s=256&x=%2&y=%3&z=%1");
            l->setMapAdapter(mapadapter);
            geomLayer->setMapAdapter(mapadapter);

            if (isVisible()) mc->updateRequestNew();
            mc->setZoom(zoom);
            overlay->setVisible(false);
            //        yahooActionOverlay->setEnabled(true);
        } else if (action == googleActionMap) {
            int zoom = mapadapter->adaptedZoom();
            mc->setZoom(0);
            mapadapter = new qmapcontrol::GoogleMapAdapter();
            l->setMapAdapter(mapadapter);
            geomLayer->setMapAdapter(mapadapter);

            if (isVisible()) mc->updateRequestNew();
            mc->setZoom(zoom);
            //        yahooActionOverlay->setEnabled(false);
            overlay->setVisible(false);
            //        yahooActionOverlay->setChecked(false);
        } else if (action == googleSatAction) {
            int zoom = mapadapter->adaptedZoom();
            mc->setZoom(0);
            mapadapter = new qmapcontrol::GoogleSatMapAdapter();
            l->setMapAdapter(mapadapter);
            geomLayer->setMapAdapter(mapadapter);

            if (isVisible()) mc->updateRequestNew();
            mc->setZoom(zoom);
            //        yahooActionOverlay->setEnabled(false);
            overlay->setVisible(false);
            //        yahooActionOverlay->setChecked(false);
        } else {
            mapButton->setText("Select..");
        }
    }
}


void MapWidget::createPathButtonClicked(bool checked)
{
    if (mc) {
        Q_UNUSED(checked);

        if (createPath->isChecked()) {
            // change the cursor shape
            this->setCursor(Qt::PointingHandCursor);
            mc->setMouseMode(qmapcontrol::MapControl::None);


            // emit signal start to create a Waypoint global
            //emit createGlobalWP(true, mc->currentCoordinate());

            //        // Clear the previous WP track
            //        // TODO: Move this to an actual clear track button and add a warning dialog
            //        mc->layer("Waypoints")->clearGeometries();
            //        wps.clear();
            //        path->setPoints(wps);
            //        mc->layer("Waypoints")->addGeometry(path);
            //        wpIndex.clear();
        } else {

            this->setCursor(Qt::ArrowCursor);
            mc->setMouseMode(qmapcontrol::MapControl::Panning);
        }
    }
}

/**
 * Captures a click on the map and if in create WP path mode, it adds the WP on MouseButtonRelease
 *
 * @param event The mouse event
 * @param coordinate The coordinate in which it occured the mouse event
 * @note  This slot is connected to the mouseEventCoordinate of the QMapControl object
 */

void MapWidget::captureMapClick(const QMouseEvent* event, const QPointF coordinate)
{
    if (QEvent::MouseButtonRelease == event->type() && createPath->isChecked()) {
        // Create waypoint name
        QString str;

        // create the WP and set everything in the LineString to display the path
        Waypoint2DIcon* tempCirclePoint;

        if (mav) {
            double altitude = 0.0;
            double yaw = 0.0;
            int wpListCount = mav->getWaypointManager()->getWaypointList().count();
            if (wpListCount > 0) {
                altitude = mav->getWaypointManager()->getWaypointList().at(wpListCount-1)->getAltitude();
                yaw = mav->getWaypointManager()->getWaypointList().at(wpListCount-1)->getYaw();
            }
            mav->getWaypointManager()->addWaypoint(new Waypoint(wpListCount, coordinate.y(), coordinate.x(), altitude, yaw, true));
        } else {
            str = QString("%1").arg(waypointPath->numberOfPoints());
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle);
            wpIcons.append(tempCirclePoint);

            mc->layer("Waypoints")->addGeometry(tempCirclePoint);

            qmapcontrol::Point* tempPoint = new qmapcontrol::Point(coordinate.x(), coordinate.y(),str);
            wps.append(tempPoint);
            waypointPath->addPoint(tempPoint);

            // Refresh the screen
            if (isVisible()) mc->updateRequest(tempPoint->boundingBox().toRect());
        }

        // emit signal mouse was clicked
        //emit captureMapCoordinateClick(coordinate);
    }
}

void MapWidget::updateWaypoint(int uas, Waypoint* wp)
{
    // Update waypoint list and redraw map (last parameter)
    updateWaypoint(uas, wp, true);
}

/**
 * This function is called if a a single waypoint is updated and
 * also if the whole list changes.
 */
void MapWidget::updateWaypoint(int uas, Waypoint* wp, bool updateView)
{
    if (mc) {
        // Make sure this is the right UAS
        if (uas == this->mav->getUASID()) {
            // Only accept waypoints in global coordinate frame
            if (wp->getFrame() == MAV_FRAME_GLOBAL && wp->isNavigationType()) {
                // We're good, this is a global waypoint

                // Get the index of this waypoint
                // note the call to getGlobalFrameAndNavTypeIndexOf()
                // as we're only handling global waypoints
                int wpindex = UASManager::instance()->getUASForId(uas)->getWaypointManager()->getGlobalFrameAndNavTypeIndexOf(wp);
                // If not found, return (this should never happen, but helps safety)
                if (wpindex == -1) return;

                // Check if wp exists yet in map
                if (!(wpIcons.count() > wpindex)) {
                    // Waypoint is new, a new icon is created
                    QPointF coordinate;
                    coordinate.setX(wp->getLongitude());
                    coordinate.setY(wp->getLatitude());
                    createWaypointGraphAtMap(wpindex, coordinate);
                } else {
                    // Waypoint exists, update it if we're not
                    // currently dragging it with the mouse
                    if(!waypointIsDrag) {
                        QPointF coordinate;
                        coordinate.setX(wp->getLongitude());
                        coordinate.setY(wp->getLatitude());

                        Point* waypoint;
                        waypoint = wps.at(wpindex);
                        if (waypoint) {
                            // First set waypoint coordinate
                            waypoint->setCoordinate(coordinate);
                            // Now update icon position
                            wpIcons.at(wpindex)->setCoordinate(coordinate);
                            // Update pen
                            wpIcons.at(wpindex)->setPen(mavPens.value(uas));
                            // Then waypoint line coordinate
                            Point* linesegment = NULL;
                            // If the line segment already exists, just update it
                            // else create a new one
                            if (waypointPath->points().size() > wpindex) {
                                linesegment = waypointPath->points().at(wpindex);
                                if (linesegment) linesegment->setCoordinate(coordinate);
                            } else {
                                waypointPath->addPoint(waypoint);
                            }

                            // Update view
                            if (updateView) if (isVisible()) mc->updateRequest(waypoint->boundingBox().toRect());
                        }
                    }
                }
            } else {
                // Check if the index of this waypoint is larger than the global
                // waypoint list. This implies that the coordinate frame of this
                // waypoint was changed and the list containing only global
                // waypoints was shortened. Thus update the whole list
                if (waypointPath->points().count() > UASManager::instance()->getUASForId(uas)->getWaypointManager()->getGlobalFrameAndNavTypeCount()) {
                    updateWaypointList(uas);
                }
            }
        }
    }
}

void MapWidget::createWaypointGraphAtMap(int id, const QPointF coordinate)
{
    //if (!wpExists(coordinate))
    {
        // Create waypoint name
        QString str;

        // create the WP and set everything in the LineString to display the path
        //CirclePoint* tempCirclePoint = new CirclePoint(coordinate.x(), coordinate.y(), 10, str);
        Waypoint2DIcon* tempCirclePoint;

        if (mav) {
            int uas = mav->getUASID();
            str = QString("%1").arg(id);
            qDebug() << "Waypoint list count:" << str;
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle, mavPens.value(uas));
        } else {
            str = QString("%1").arg(id);
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle);
        }


        mc->layer("Waypoints")->addGeometry(tempCirclePoint);
        wpIcons.append(tempCirclePoint);

        Point* tempPoint = new Point(coordinate.x(), coordinate.y(),str);
        wps.append(tempPoint);
        waypointPath->addPoint(tempPoint);

        //wpIndex.insert(str,tempPoint);
        qDebug()<<"Funcion createWaypointGraphAtMap WP= "<<str<<" -> x= "<<tempPoint->latitude()<<" y= "<<tempPoint->longitude();

        // Refresh the screen
        if (isVisible()) if (isVisible()) mc->updateRequest(tempPoint->boundingBox().toRect());
    }

    ////    // emit signal mouse was clicked
    //    emit captureMapCoordinateClick(coordinate);
}

int MapWidget::wpExists(const QPointF coordinate)
{
    if (mc) {
        for (int i = 0; i < wps.size(); i++) {
            if (wps.at(i)->latitude() == coordinate.y() &&
                    wps.at(i)->longitude()== coordinate.x()) {
                return 1;
            }
        }
    }
    return 0;
}


void MapWidget::captureGeometryClick(Geometry* geom, QPoint point)
{
    Q_UNUSED(geom);
    Q_UNUSED(point);

    if (mc) mc->setMouseMode(qmapcontrol::MapControl::None);
}

void MapWidget::captureGeometryDrag(Geometry* geom, QPointF coordinate)
{
    waypointIsDrag = true;

    // Refresh the screen
    if (isVisible()) mc->updateRequest(geom->boundingBox().toRect());

    int temp = 0;

    // Get waypoint index in list
    bool wpIndexOk;
    int index = geom->name().toInt(&wpIndexOk);

    Waypoint2DIcon* point2Find = dynamic_cast <Waypoint2DIcon*> (geom);

    if (wpIndexOk && point2Find && wps.count() > index) {
        // Update visual
        point2Find->setCoordinate(coordinate);
        waypointPath->points().at(index)->setCoordinate(coordinate);
        if (isVisible()) mc->updateRequest(waypointPath->boundingBox().toRect());

        // Update waypoint data storage
        if (mav) {
            QVector<Waypoint*> wps = mav->getWaypointManager()->getGlobalFrameAndNavTypeWaypointList();

            if (wps.size() > index) {
                Waypoint* wp = wps.at(index);
                wp->setLatitude(coordinate.y());
                wp->setLongitude(coordinate.x());
                mav->getWaypointManager()->notifyOfChange(wp);
            }
        }

        // qDebug() << geom->name();
        temp = geom->get_myIndex();
        //qDebug() << temp;
        emit sendGeometryEndDrag(coordinate,temp);
    }

    waypointIsDrag = false;
}

void MapWidget::captureGeometryEndDrag(Geometry* geom, QPointF coordinate)
{
    Q_UNUSED(geom);
    Q_UNUSED(coordinate);
    // TODO: Investigate why when creating the waypoint path this slot is being called

    // Only change the mouse mode back to panning when not creating a WP path
    if (!createPath->isChecked()) {
        waypointIsDrag = false;
        mc->setMouseMode(qmapcontrol::MapControl::Panning);
    }

}

MapWidget::~MapWidget()
{
    delete mc;
    delete m_ui;
}
/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void MapWidget::addUAS(UASInterface* uas)
{
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(systemSpecsChanged(int)), this, SLOT(updateSystemSpecs(int)));
}

/**
 * Update the whole list of waypoints. This is e.g. necessary if the list order changed.
 * The UAS manager will emit the appropriate signal whenever updating the list
 * is necessary.
 */
void MapWidget::updateWaypointList(int uas)
{
    if (mc) {
        // Get already existing waypoints
        UASInterface* uasInstance = UASManager::instance()->getUASForId(uas);
        if (uasInstance) {
            // Get update rect of old content, this is what will be redrawn
            // in the last step
            QRect updateRect = waypointPath->boundingBox().toRect();

            // Get all waypoints, including non-global waypoints
            QVector<Waypoint*> wpList = uasInstance->getWaypointManager()->getWaypointList();

            // Clear if necessary
            if (wpList.count() == 0) {
                clearWaypoints(uas);
                return;
            }

            // Trim internal list to number of global waypoints in the waypoint manager list
            int overSize = waypointPath->points().count() - uasInstance->getWaypointManager()->getGlobalFrameAndNavTypeCount();
            if (overSize > 0) {
                // Remove n waypoints at the end of the list
                // the remaining waypoints will be updated
                // in the next step
                for (int i = 0; i < overSize; ++i) {
                    wps.removeLast();
                    mc->layer("Waypoints")->removeGeometry(wpIcons.last());
                    wpIcons.removeLast();
                    waypointPath->points().removeLast();
                }
            }

            // Load all existing waypoints into map view
            foreach (Waypoint* wp, wpList) {
                // Block map draw updates, since we update everything in the next step
                // but update internal data structures.
                // Please note that updateWaypoint() ignores non-global waypoints
                updateWaypoint(mav->getUASID(), wp, false);
            }

            // Update view
            if (isVisible()) mc->updateRequest(updateRect);
        }
    }
}

void MapWidget::redoWaypoints(int uas)
{
    //    QObject* sender = QObject::sender();
    //    UASWaypointManager* manager = dynamic_cast<UASWaypointManager*>(sender);
    //    if (sender)
    //    {
    // Get waypoint list for this MAV

    // Clear all waypoints
    //clearWaypoints();
    // Re-add the updated waypoints

    //    }

    updateWaypointList(uas);
}

void MapWidget::activeUASSet(UASInterface* uas)
{
    // Disconnect old MAV
    if (mav) {
        // Disconnect the waypoint manager / data storage from the UI
        disconnect(mav->getWaypointManager(), SIGNAL(waypointListChanged(int)), this, SLOT(updateWaypointList(int)));
        disconnect(mav->getWaypointManager(), SIGNAL(waypointChanged(int, Waypoint*)), this, SLOT(updateWaypoint(int,Waypoint*)));
        disconnect(this, SIGNAL(waypointCreated(Waypoint*)), mav->getWaypointManager(), SLOT(addWaypoint(Waypoint*)));
    }

    if (uas && mc) {
        mav = uas;
        QColor color = mav->getColor();
        color.setAlphaF(0.9);
        QPen* pen = new QPen(color);
        pen->setWidth(3.0);
        mavPens.insert(mav->getUASID(), pen);
        // FIXME Remove after refactoring
        waypointPath->setPen(pen);

        // Delete all waypoints and add waypoint from new system
        //redoWaypoints();
        updateWaypointList(uas->getUASID());

        // Connect the waypoint manager / data storage to the UI
        connect(mav->getWaypointManager(), SIGNAL(waypointListChanged(int)), this, SLOT(updateWaypointList(int)));
        connect(mav->getWaypointManager(), SIGNAL(waypointChanged(int, Waypoint*)), this, SLOT(updateWaypoint(int,Waypoint*)));
        connect(this, SIGNAL(waypointCreated(Waypoint*)), mav->getWaypointManager(), SLOT(addWaypoint(Waypoint*)));

        updateSystemSpecs(mav->getUASID());
        updateSelectedSystem(mav->getUASID());
        mc->updateRequest(waypointPath->boundingBox().toRect());
    }
}

void MapWidget::updateSystemSpecs(int uas)
{
    if (mc) {
        foreach (qmapcontrol::Point* p, uasIcons.values()) {
            MAV2DIcon* icon = dynamic_cast<MAV2DIcon*>(p);
            if (icon && icon->getUASId() == uas) {
                // Set new airframe
                icon->setAirframe(UASManager::instance()->getUASForId(uas)->getAirframe());
                icon->drawIcon();
            }
        }
    }
}

void MapWidget::updateSelectedSystem(int uas)
{
    if (mc) {
        foreach (qmapcontrol::Point* p, uasIcons.values()) {
            MAV2DIcon* icon = dynamic_cast<MAV2DIcon*>(p);
            if (icon) {
                // Set as selected if ids match
                icon->setSelectedUAS((icon->getUASId() == uas));
            }
        }
    }
}

void MapWidget::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 usec)
{
    Q_UNUSED(roll);
    Q_UNUSED(pitch);
    Q_UNUSED(usec);
    if (mc) {

        if (uas) {
            MAV2DIcon* icon = dynamic_cast<MAV2DIcon*>(uasIcons.value(uas->getUASID(), NULL));
            if (icon) {
                icon->setYaw(yaw);
            }
        }
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
void MapWidget::updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec)
{
    Q_UNUSED(usec);
    Q_UNUSED(alt); // FIXME Use altitude
    if (mc) {
        // create a LineString
        //QList<Point*> points;
        // Points with a circle
        // A QPen can be used to customize the
        //pointpen->setWidth(3);
        //points.append(new CirclePoint(lat, lon, 10, uas->getUASName(), Point::Middle, pointpen));

        qmapcontrol::Point* p;
        QPointF coordinate;
        coordinate.setX(lon);
        coordinate.setY(lat);

        if (!uasIcons.contains(uas->getUASID())) {
            // Get the UAS color
            QColor uasColor = uas->getColor();

            // Icon
            //QPen* pointpen = new QPen(uasColor);
            qDebug() << "2D MAP: ADDING" << uas->getUASName() << __FILE__ << __LINE__;
            p = new MAV2DIcon(uas, 68, uas->getSystemType(), uas->getColor(), QString("%1").arg(uas->getUASID()), qmapcontrol::Point::Middle);
            uasIcons.insert(uas->getUASID(), p);
            mc->layer("Waypoints")->addGeometry(p);

            // Line
            // A QPen also can use transparency

            //        QList<qmapcontrol::Point*> points;
            //        points.append(new qmapcontrol::Point(coordinate.x(), coordinate.y()));
            //        QPen* linepen = new QPen(uasColor.darker());
            //        linepen->setWidth(2);

            //        // Create tracking line string
            //        qmapcontrol::LineString* ls = new qmapcontrol::LineString(points, QString("%1").arg(uas->getUASID()), linepen);
            //        uasTrails.insert(uas->getUASID(), ls);

            //        // Add the LineString to the layer
            //        mc->layer("Waypoints")->addGeometry(ls);
        } else {
            //        p = dynamic_cast<MAV2DIcon*>(uasIcons.value(uas->getUASID()));
            //        if (p)
            //        {
            p = uasIcons.value(uas->getUASID());
            p->setCoordinate(QPointF(lon, lat));
            //p->setYaw(uas->getYaw());
            //        }
            // Extend trail
            //        uasTrails.value(uas->getUASID())->addPoint(new qmapcontrol::Point(coordinate.x(), coordinate.y()));
        }

        if (isVisible()) mc->updateRequest(p->boundingBox().toRect());

        //if (isVisible()) mc->updateRequestNew();//(uasTrails.value(uas->getUASID())->boundingBox().toRect());

        if (this->mav && uas->getUASID() == this->mav->getUASID()) {
            // Limit the position update rate
            quint64 currTime = MG::TIME::getGroundTimeNow();
            if (currTime - lastUpdate > 120) {
                lastUpdate = currTime;
                // Sets the view to the interesting area
                if (followgps->isChecked()) {
                    updatePosition(0, lon, lat);
                } else {
                    // Refresh the screen
                    //if (isVisible()) mc->updateRequestNew();
                }
            }
        }
    }
}

/**
 * Center the view on this position
 */
void MapWidget::updatePosition(float time, double lat, double lon)
{
    Q_UNUSED(time);
    //gpsposition->setText(QString::number(time) + " / " + QString::number(lat) + " / " + QString::number(lon));
    if (followgps->isChecked() && isVisible() && mc) {
        if (mc) mc->setView(QPointF(lat, lon));
    }
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
    if (mc) {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        // Calculate new zoom level
        int newZoom = mc->currentZoom()+numSteps;
        // Set new zoom level, level is bounded by map control
        mc->setZoom(newZoom);
        // Detail zoom level is the number of steps zoomed in further
        // after the bounding has taken effect
        detailZoom = qAbs(qMin(0, mc->currentZoom()-newZoom));

        // visual field of camera
        updateCameraPosition(20*newZoom,0,"no");
    }
}

void MapWidget::keyPressEvent(QKeyEvent *event)
{
    if (mc) {
        switch (event->key()) {
        case Qt::Key_Plus:
            mc->zoomIn();
            break;
        case Qt::Key_Minus:
            mc->zoomOut();
            break;
        case Qt::Key_Left:
            mc->scrollLeft(this->width()/scrollStep);
            break;
        case Qt::Key_Right:
            mc->scrollRight(this->width()/scrollStep);
            break;
        case Qt::Key_Down:
            mc->scrollDown(this->width()/scrollStep);
            break;
        case Qt::Key_Up:
            mc->scrollUp(this->width()/scrollStep);
            break;
        default:
            QWidget::keyPressEvent(event);
        }
    }
}

void MapWidget::resizeEvent(QResizeEvent* event )
{
    Q_UNUSED(event);
    if (!initialized) {
        init();
    }
    if (mc) mc->resize(this->size());
}

void MapWidget::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
//    if (isVisible())
//    {
//	if (!initialized)
//	{
//            init();
//	}
//    }
}

void MapWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    if (mc) {
        QSettings settings;
        settings.beginGroup("QGC_MAPWIDGET");
        QPointF currentPos = mc->currentCoordinate();
        settings.setValue("LAST_LATITUDE", currentPos.y());
        settings.setValue("LAST_LONGITUDE", currentPos.x());
        settings.setValue("LAST_ZOOM", mc->currentZoom());
        settings.endGroup();
        settings.sync();
    }
}


void MapWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MapWidget::clearWaypoints(int uas)
{
    if (mc) {
        Q_UNUSED(uas);
        // Clear the previous WP track

        //mc->layer("Waypoints")->clearGeometries();
        wps.clear();
        foreach (Point* p, wpIcons) {
            mc->layer("Waypoints")->removeGeometry(p);
        }
        wpIcons.clear();

        // Get bounding box of this object BEFORE deleting the content
        QRect box = waypointPath->boundingBox().toRect();

        // Delete the content
        waypointPath->points().clear();

        //delete waypointPath;
        //waypointPath = new
        //mc->layer("Waypoints")->addGeometry(waypointPath);
        //wpIndex.clear();
        if (isVisible()) mc->updateRequest(box);//(waypointPath->boundingBox().toRect());

        if(createPath->isChecked()) {
            createPath->click();
        }
    }
}

void MapWidget::clearPath(int uas)
{
    Q_UNUSED(uas);
    if (mc) {
        mc->layer("Tracking")->clearGeometries();
        foreach (qmapcontrol::LineString* ls, uasTrails) {
            QPen* linepen = ls->pen();
            delete ls;
            qmapcontrol::LineString* lsNew = new qmapcontrol::LineString(QList<qmapcontrol::Point*>(), "", linepen);
            mc->layer("Tracking")->addGeometry(lsNew);
        }
        // FIXME update this with update request only for bounding box of trails
        if (isVisible()) mc->updateRequestNew();//(QRect(0, 0, width(), height()));
    }
}

void MapWidget::updateCameraPosition(double radio, double bearing, QString dir)
{
    Q_UNUSED(dir);
    Q_UNUSED(bearing);
    if (mc) {
        // FIXME Mariano
        //camPoints.clear();
        QPointF currentPos = mc->currentCoordinate();
        //    QPointF actualPos = getPointxBearing_Range(currentPos.y(),currentPos.x(),bearing,distance);

        //    qmapcontrol::Point* tempPoint1 = new qmapcontrol::Point(currentPos.x(), currentPos.y(),"inicial",qmapcontrol::Point::Middle);
        //    qmapcontrol::Point* tempPoint2 = new qmapcontrol::Point(actualPos.x(), actualPos.y(),"final",qmapcontrol::Point::Middle);

        //    camPoints.append(tempPoint1);
        //    camPoints.append(tempPoint2);

        //    camLine->setPoints(camPoints);

        QPen* camBorderPen = new QPen(QColor(255,0,0));
        camBorderPen->setWidth(2);

        //radio = mc->currentZoom()

        if(drawCamBorder) {
            //clear camera borders
            mc->layer("Camera")->clearGeometries();

            //create a camera borders
            qmapcontrol::CirclePoint* camBorder = new qmapcontrol::CirclePoint(currentPos.x(), currentPos.y(), radio, "camBorder", qmapcontrol::Point::Middle, camBorderPen);

            //camBorder->setCoordinate(currentPos);

            mc->layer("Camera")->addGeometry(camBorder);
            // mc->layer("Camera")->addGeometry(camLine);
            if (isVisible()) mc->updateRequestNew();

        } else {
            //clear camera borders
            mc->layer("Camera")->clearGeometries();
            if (isVisible()) mc->updateRequestNew();

        }
    }
}

void MapWidget::drawBorderCamAtMap(bool status)
{
    drawCamBorder = status;
    updateCameraPosition(20,0,"no");

}

QPointF MapWidget::getPointxBearing_Range(double lat1, double lon1, double bearing, double distance)
{
    QPointF temp;

    double rad = M_PI/180;

    bearing = bearing*rad;
    temp.setX((lon1 + ((distance/60) * (sin(bearing)))));
    temp.setY((lat1 + ((distance/60) * (cos(bearing)))));

    return temp;
}

