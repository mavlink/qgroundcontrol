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
#include <QDoubleSpinBox>

#include "QGC.h"
#include "MapWidget.h"
#include "ui_MapWidget.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "MAV2DIcon.h"
#include "Waypoint2DIcon.h"

#include "MG.h"


MapWidget::MapWidget(QWidget *parent) :
        QWidget(parent),
        zoomLevel(0),
        uasIcons(),
        uasTrails(),
        mav(NULL),
        lastUpdate(0),
        m_ui(new Ui::MapWidget)
{
    m_ui->setupUi(this);
    mc = new qmapcontrol::MapControl(QSize(320, 240));

    //   VISUAL MAP STYLE
    QString buttonStyle("QAbstractButton { background-color: rgba(20, 20, 20, 45%); border-color: rgba(10, 10, 10, 50%)}  QDoubleSpinBox { background-color: rgba(20, 20, 20, 45%); border-color: rgba(10, 10, 10, 50%)}");
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

    // SET INITIAL POSITION AND ZOOM
    // Set default zoom level
    mc->setZoom(16);
    // Zurich, ETH
    mc->setView(QPointF(8.548056,47.376389));

    // Veracruz Mexico
    //mc->setView(QPointF(-96.105208,19.138955));

    // Add controls to select map provider
    /////////////////////////////////////////////////
    QActionGroup* mapproviderGroup = new QActionGroup(this);
    osmAction = new QAction(QIcon(":/images/mapproviders/openstreetmap.png"), tr("OpenStreetMap"), mapproviderGroup);
    yahooActionMap = new QAction(QIcon(":/images/mapproviders/yahoo.png"), tr("Yahoo: Map"), mapproviderGroup);
    yahooActionSatellite = new QAction(QIcon(":/images/mapproviders/yahoo.png"), tr("Yahoo: Satellite"), mapproviderGroup);
    googleActionMap = new QAction(QIcon(":/images/mapproviders/google.png"), tr("Google: Map"), mapproviderGroup);
    googleSatAction = new QAction(QIcon(":/images/mapproviders/google.png"), tr("Google: Sat"), mapproviderGroup);
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

    // display the MapControl in the application
    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(mc, 0, 0);
    setLayout(layout);

    // create buttons to control the map (zoom, GPS tracking and WP capture)
    QPushButton* zoomin = new QPushButton(QIcon(":/images/actions/list-add.svg"), "", this);
    zoomin->setStyleSheet(buttonStyle);
    QPushButton* zoomout = new QPushButton(QIcon(":/images/actions/list-remove.svg"), "", this);
    zoomout->setStyleSheet(buttonStyle);
    createPath = new QPushButton(QIcon(":/images/actions/go-bottom.svg"), "", this);
    createPath->setStyleSheet(buttonStyle);
    clearTracking = new QPushButton(QIcon(""), "", this);
    clearTracking->setStyleSheet(buttonStyle);
    followgps = new QPushButton(QIcon(":/images/actions/system-lock-screen.svg"), "", this);
    followgps->setStyleSheet(buttonStyle);
    QPushButton* goToButton = new QPushButton(QIcon(""), "T", this);
    goToButton->setStyleSheet(buttonStyle);

    // SAVE FILES
    QPushButton* saveButton = new QPushButton(QIcon(""), "S", this);
    saveButton->setStyleSheet(buttonStyle);

    // SET OFFSCREEN BUFFER SIZE
    QDoubleSpinBox* offscreenSpinBox = new QDoubleSpinBox(this);
    offscreenSpinBox->setStyleSheet(buttonStyle);
    offscreenSpinBox->setMinimum(2);
    offscreenSpinBox->setMaximum(30);
    offscreenSpinBox->setValue(mc->offscreenImageFactor());
    connect(offscreenSpinBox, SIGNAL(valueChanged(double)), mc, SLOT(setOffscreenImageFactor(double)));


    zoomin->setMaximumWidth(30);
    zoomout->setMaximumWidth(30);
    createPath->setMaximumWidth(30);
    clearTracking->setMaximumWidth(30);
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
    innerlayout->addWidget(clearTracking, 4, 0);
    // Add spacers to compress buttons on the top left
    innerlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 5, 0);
    innerlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, 1, 0, 7);
    innerlayout->addWidget(mapButton, 0, 2);
    innerlayout->addWidget(goToButton, 0, 3);
    innerlayout->addWidget(saveButton, 0, 4);
    innerlayout->addWidget(offscreenSpinBox, 0, 5);
    innerlayout->setRowStretch(0, 1);
    innerlayout->setRowStretch(1, 100);
    mc->setLayout(innerlayout);


    // Connect the required signals-slots
    connect(zoomin, SIGNAL(clicked(bool)),
            mc, SLOT(zoomIn()));

    connect(zoomout, SIGNAL(clicked(bool)),
            mc, SLOT(zoomOut()));

    connect(goToButton, SIGNAL(clicked()), this, SLOT(goTo()));

    connect(saveButton, SIGNAL(clicked()), mc, SLOT(openImageSaveDialog()));

    QList<UASInterface*> systems = UASManager::instance()->getUASList();
    foreach(UASInterface* system, systems)
    {
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

    //mc->setOffscreenImageFactor(15);
}

void MapWidget::goTo()
{
    bool ok;
         QString text = QInputDialog::getText(this, tr("Please enter coordinates"),
                                              tr("Coordinates (Lat,Lon):"), QLineEdit::Normal,
                                              QString("%1,%2").arg(mc->currentCoordinate().x()).arg(mc->currentCoordinate().y()), &ok);
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
                     mc->setView(QPointF(latitude, longitude));
                 }
             }
         }
}


void MapWidget::mapproviderSelected(QAction* action)
{
    //delete mapadapter;
    mapButton->setText(action->text());
    if (action == osmAction)
    {
        int zoom = mapadapter->adaptedZoom();
        mc->setZoom(0);

        mapadapter = new qmapcontrol::OSMMapAdapter();
        l->setMapAdapter(mapadapter);
        geomLayer->setMapAdapter(mapadapter);

        mc->updateRequestNew();
        mc->setZoom(zoom);
        //        yahooActionOverlay->setEnabled(false);
        overlay->setVisible(false);
        //        yahooActionOverlay->setChecked(false);

    }
    else if (action == yahooActionMap)
    {
        int zoom = mapadapter->adaptedZoom();
        mc->setZoom(0);

        mapadapter = new qmapcontrol::YahooMapAdapter();
        l->setMapAdapter(mapadapter);
        geomLayer->setMapAdapter(mapadapter);

        mc->updateRequestNew();
        mc->setZoom(zoom);
        //        yahooActionOverlay->setEnabled(false);
        overlay->setVisible(false);
        //        yahooActionOverlay->setChecked(false);
    }
    else if (action == yahooActionSatellite)
    {
        int zoom = mapadapter->adaptedZoom();
        QPointF a = mc->currentCoordinate();
        mc->setZoom(0);

        mapadapter = new qmapcontrol::YahooMapAdapter("us.maps3.yimg.com", "/aerial.maps.yimg.com/png?v=1.7&t=a&s=256&x=%2&y=%3&z=%1");
        l->setMapAdapter(mapadapter);

        mc->updateRequestNew();
        mc->setZoom(zoom);
        //        yahooActionOverlay->setEnabled(true);
    }
    else if (action == googleActionMap)
    {
        int zoom = mapadapter->adaptedZoom();
        mc->setZoom(0);
        mapadapter = new qmapcontrol::GoogleMapAdapter();
        l->setMapAdapter(mapadapter);
        geomLayer->setMapAdapter(mapadapter);

        mc->updateRequestNew();
        mc->setZoom(zoom);
        //        yahooActionOverlay->setEnabled(false);
        overlay->setVisible(false);
        //        yahooActionOverlay->setChecked(false);
    }
    else if (action == googleSatAction)
    {
        int zoom = mapadapter->adaptedZoom();
        mc->setZoom(0);
        mapadapter = new qmapcontrol::GoogleSatMapAdapter();
        l->setMapAdapter(mapadapter);
        geomLayer->setMapAdapter(mapadapter); 

        mc->updateRequestNew();
        mc->setZoom(zoom);
        //        yahooActionOverlay->setEnabled(false);
        overlay->setVisible(false);
        //        yahooActionOverlay->setChecked(false);
    }
    else
    {
        mapButton->setText("Select..");
    }
}


void MapWidget::createPathButtonClicked(bool checked)
{
    Q_UNUSED(checked);

    if (createPath->isChecked())
    {
        // change the cursor shape
        this->setCursor(Qt::PointingHandCursor);
        mc->setMouseMode(qmapcontrol::MapControl::None);


        // emit signal start to create a Waypoint global
        emit createGlobalWP(true, mc->currentCoordinate());

        //        // Clear the previous WP track
        //        // TODO: Move this to an actual clear track button and add a warning dialog
        //        mc->layer("Waypoints")->clearGeometries();
        //        wps.clear();
        //        path->setPoints(wps);
        //        mc->layer("Waypoints")->addGeometry(path);
        //        wpIndex.clear();
    }
    else
    {

        this->setCursor(Qt::ArrowCursor);
        mc->setMouseMode(qmapcontrol::MapControl::Panning);
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
    //qDebug() << mc->mouseMode();
    if (QEvent::MouseButtonRelease == event->type() && createPath->isChecked())
    {
        // Create waypoint name
        QString str;

        str = QString("%1").arg(waypointPath->numberOfPoints());

        // create the WP and set everything in the LineString to display the path
        Waypoint2DIcon* tempCirclePoint;

        if (mav)
        {
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle, new QPen(mav->getColor()));
        }
        else
        {
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle);
        }
        mc->layer("Waypoints")->addGeometry(tempCirclePoint);

        qmapcontrol::Point* tempPoint = new qmapcontrol::Point(coordinate.x(), coordinate.y(),str);
        wps.append(tempPoint);
        waypointPath->addPoint(tempPoint);

        wpIndex.insert(str,tempPoint);

        // Refresh the screen
        mc->updateRequest(tempPoint->boundingBox().toRect());

        // emit signal mouse was clicked
        emit captureMapCoordinateClick(coordinate);
    }
}

void MapWidget::createWaypointGraphAtMap(const QPointF coordinate)
{
    if (!wpExists(coordinate))
    {
        // Create waypoint name
        QString str;


        str = QString("%1").arg(waypointPath->numberOfPoints());

        // create the WP and set everything in the LineString to display the path
        //CirclePoint* tempCirclePoint = new CirclePoint(coordinate.x(), coordinate.y(), 10, str);
        Waypoint2DIcon* tempCirclePoint;

        if (mav)
        {
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle, new QPen(mav->getColor()));
        }
        else
        {
            tempCirclePoint = new Waypoint2DIcon(coordinate.x(), coordinate.y(), 20, str, qmapcontrol::Point::Middle);
        }


        mc->layer("Waypoints")->addGeometry(tempCirclePoint);

        Point* tempPoint = new Point(coordinate.x(), coordinate.y(),str);
        wps.append(tempPoint);
        waypointPath->addPoint(tempPoint);

        wpIndex.insert(str,tempPoint);
        qDebug()<<"Funcion createWaypointGraphAtMap WP= "<<str<<" -> x= "<<tempPoint->latitude()<<" y= "<<tempPoint->longitude();

        // Refresh the screen
        mc->updateRequest(tempPoint->boundingBox().toRect());
    }

    ////    // emit signal mouse was clicked
    //    emit captureMapCoordinateClick(coordinate);
}

int MapWidget::wpExists(const QPointF coordinate){
    for (int i = 0; i < wps.size(); i++){
        if (wps.at(i)->latitude() == coordinate.y() &&
            wps.at(i)->longitude()== coordinate.x()){
            return 1;
        }
    }
    return 0;
}


void MapWidget::captureGeometryClick(Geometry* geom, QPoint point)
{
    Q_UNUSED(geom);
    Q_UNUSED(point);

    mc->setMouseMode(qmapcontrol::MapControl::None);

}

void MapWidget::captureGeometryDrag(Geometry* geom, QPointF coordinate)
{


    waypointIsDrag = true;

    // Refresh the screen
    mc->updateRequest(geom->boundingBox().toRect());

    int temp = 0;
    qmapcontrol::Point* point2Find;
    point2Find = wpIndex[geom->name()];

    if (point2Find)
    {
        point2Find->setCoordinate(coordinate);

        point2Find = dynamic_cast <qmapcontrol::Point*> (geom);
        if (point2Find)
        {
            point2Find->setCoordinate(coordinate);

            // qDebug() << geom->name();
            temp = geom->get_myIndex();
            //qDebug() << temp;
            emit sendGeometryEndDrag(coordinate,temp);
        }
    }

}

void MapWidget::captureGeometryEndDrag(Geometry* geom, QPointF coordinate)
{
    Q_UNUSED(geom);
    Q_UNUSED(coordinate);
    // TODO: Investigate why when creating the waypoint path this slot is being called

    // Only change the mouse mode back to panning when not creating a WP path
    if (!createPath->isChecked())
    {
        waypointIsDrag = false;
        mc->setMouseMode(qmapcontrol::MapControl::Panning);
    }

}

MapWidget::~MapWidget()
{
    delete m_ui;
}
/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void MapWidget::addUAS(UASInterface* uas)
{
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
}

void MapWidget::activeUASSet(UASInterface* uas)
{
    if (uas)
    {
        mav = uas;
        QColor color = mav->getColor();
        color.setAlphaF(0.6);
        QPen* pen = new QPen(color);
        pen->setWidth(3.0);
        // FIXME Load waypoints of this system
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

    // create a LineString
    //QList<Point*> points;
    // Points with a circle
    // A QPen can be used to customize the
    //pointpen->setWidth(3);
    //points.append(new CirclePoint(lat, lon, 10, uas->getUASName(), Point::Middle, pointpen));

    MAV2DIcon* p;

    if (!uasIcons.contains(uas->getUASID()))
    {
        // Get the UAS color
        QColor uasColor = uas->getColor();

        // Icon
        QPen* pointpen = new QPen(uasColor);
        qDebug() << uas->getUASName();
        p = new MAV2DIcon(lat, lon, 20, uas->getUASName(), qmapcontrol::Point::Middle, pointpen);
        uasIcons.insert(uas->getUASID(), p);
        tracks->addGeometry(p);

        // Line
        // A QPen also can use transparency

        QList<qmapcontrol::Point*> points;
        points.append(new qmapcontrol::Point(lat, lon, ""));
        QPen* linepen = new QPen(uasColor.darker());
        linepen->setWidth(2);

        // Create tracking line string
        qmapcontrol::LineString* ls = new qmapcontrol::LineString(points, uas->getUASName(), linepen);
        uasTrails.insert(uas->getUASID(), ls);

        // Add the LineString to the layer
        mc->layer("Tracking")->addGeometry(ls);
    }
    else
    {
        p = dynamic_cast<MAV2DIcon*>(uasIcons.value(uas->getUASID()));
        if (p)
        {
            p->setCoordinate(QPointF(lat, lon));
            p->setYaw(uas->getYaw());
        }
        // Extend trail
        uasTrails.value(uas->getUASID())->addPoint(new qmapcontrol::Point(lat, lon, ""));
    }

    mc->drawGeometries();

    //mc->updateRequest(QRect(QPoint(0, 0), QPoint(600, 600)));

    //mc->updateRequestNew();//(uasTrails.value(uas->getUASID())->boundingBox().toRect());


    // Limit the position update rate
    quint64 currTime = MG::TIME::getGroundTimeNow();
    if (currTime - lastUpdate > 120)
    {
        lastUpdate = currTime;
        // Sets the view to the interesting area
        if (followgps->isChecked())
        {
            updatePosition(0, lat, lon);
        }
        else
        {
            // Refresh the screen
            //mc->updateRequestNew();
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
    if (followgps->isChecked() && isVisible())
    {
        mc->setView(QPointF(lat, lon));
    }
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
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

void MapWidget::keyPressEvent(QKeyEvent *event)
{
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

void MapWidget::resizeEvent(QResizeEvent* event )
{
    Q_UNUSED(event);
    mc->resize(this->size());
}

void MapWidget::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
}

void MapWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
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

void MapWidget::clearWaypoints()
{
    // Clear the previous WP track

    mc->layer("Waypoints")->clearGeometries();
    wps.clear();
    waypointPath->setPoints(wps);
    mc->layer("Waypoints")->addGeometry(waypointPath);
    wpIndex.clear();
    mc->updateRequest(waypointPath->boundingBox().toRect());

    if(createPath->isChecked())
    {
        createPath->click();
    }
}

void MapWidget::clearPath()
{
    mc->layer("Tracking")->clearGeometries();
    foreach (qmapcontrol::LineString* ls, uasTrails)
    {
        QPen* linepen = ls->pen();
        delete ls;
        qmapcontrol::LineString* lsNew = new qmapcontrol::LineString(QList<qmapcontrol::Point*>(), "", linepen);
        mc->layer("Tracking")->addGeometry(lsNew);
    }
    // FIXME update this with update request only for bounding box of trails
    mc->updateRequest(QRect(0, 0, width(), height()));
}

void MapWidget::changeGlobalWaypointPositionBySpinBox(int index, float lat, float lon)
{
    if(!waypointIsDrag)
    {
        qDebug() <<"indice WP= "<<index <<"\n";

        QPointF coordinate;
        coordinate.setX(lon);
        coordinate.setY(lat);

        Point* point2Find;
        point2Find = wpIndex[QString::number(index)];
        point2Find->setCoordinate(coordinate);

        point2Find = dynamic_cast <Point*> (mc->layer("Waypoints")->get_Geometry(index));
        point2Find->setCoordinate(coordinate);

        // Refresh the screen
        mc->updateRequest(point2Find->boundingBox().toRect());
    }


}

void MapWidget::updateCameraPosition(double radio, double bearing, QString dir)
{
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

    if(drawCamBorder)
    {
        //clear camera borders
        mc->layer("Camera")->clearGeometries();

        //create a camera borders
        qmapcontrol::CirclePoint* camBorder = new qmapcontrol::CirclePoint(currentPos.x(), currentPos.y(), radio, "camBorder", qmapcontrol::Point::Middle, camBorderPen);

        //camBorder->setCoordinate(currentPos);

        mc->layer("Camera")->addGeometry(camBorder);
        // mc->layer("Camera")->addGeometry(camLine);
        mc->updateRequestNew();

    }
    else
    {
        //clear camera borders
        mc->layer("Camera")->clearGeometries();
        mc->updateRequestNew();

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

