/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

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

#include "MapWidget.h"
#include "ui_MapWidget.h"
#include "UASInterface.h"
#include "UASManager.h"

#include "MG.h"

MapWidget::MapWidget(QWidget *parent) :
        QWidget(parent),
        zoomLevel(0),
        uasIcons(),
        uasTrails(),
        m_ui(new Ui::MapWidget)
{
    m_ui->setupUi(this);

    // Accept focus by clicking or keyboard
    this->setFocusPolicy(Qt::StrongFocus);

    // create MapControl
    mc = new MapControl(QSize(320, 240));
    mc->showScale(true);
    mc->showCoord(true);
    mc->enablePersistentCache();
    mc->setMouseTracking(true); // required to update the mouse position for diplay and capture

    // create MapAdapter to get maps from
    //TileMapAdapter* osmAdapter = new TileMapAdapter("tile.openstreetmap.org", "/%1/%2/%3.png", 256, 0, 17);

    MapAdapter* mapadapter_overlay = new YahooMapAdapter("us.maps3.yimg.com", "/aerial.maps.yimg.com/png?v=2.2&t=h&s=256&x=%2&y=%3&z=%1");

    // MAP BACKGROUND
    mapadapter = new GoogleSatMapAdapter();
    l = new MapLayer("Google Satellite", mapadapter);
    mc->addLayer(l);

    // STREET OVERLAY
    overlay = new MapLayer("Overlay", mapadapter_overlay);
    overlay->setVisible(false);
    mc->addLayer(overlay);

    // WAYPOINT LAYER
    // create a layer with the mapadapter and type GeometryLayer (for waypoints)
    geomLayer = new GeometryLayer("Waypoints", mapadapter);
    mc->addLayer(geomLayer);

//
//    Layer* gsatLayer = new Layer("Google Satellite", gsat, Layer::MapLayer);
//    mc->addLayer(gsatLayer);

    // SET INITIAL POSITION AND ZOOM
    // Set default zoom level
    mc->setZoom(16);
    // Zurich, ETH
    mc->setView(QPointF(8.548056,47.376389));

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

    // display the MapControl in the application
    QGridLayout* layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);
    layout->addWidget(mc, 0, 0, 1, 2);
    layout->addWidget(mapButton, 1, 0);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 1);
    layout->setRowStretch(0, 100);
    layout->setRowStretch(1, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 50);
    setLayout(layout);

    // create buttons to control the map (zoom, GPS tracking and WP capture)
    QPushButton* zoomin = new QPushButton(QIcon(":/images/actions/list-add.svg"), "", this);
    QPushButton* zoomout = new QPushButton(QIcon(":/images/actions/list-remove.svg"), "", this);
    createPath = new QPushButton(QIcon(":/images/actions/go-bottom.svg"), "", this);
    followgps = new QPushButton(QIcon(":/images/actions/system-lock-screen.svg"), "", this);

    zoomin->setMaximumWidth(50);
    zoomout->setMaximumWidth(50);
    createPath->setMaximumWidth(50);
    followgps->setMaximumWidth(50);

    // Set checkable buttons
    // TODO: Currently checked buttons are are very difficult to distinguish when checked.
    //       create a style and the slots to change the background so it is easier to distinguish
    followgps->setCheckable(true);
    createPath->setCheckable(true);

    // add buttons to control the map (zoom, GPS tracking and WP capture)
    QGridLayout* innerlayout = new QGridLayout(mc);
    innerlayout->setMargin(5);
    innerlayout->setSpacing(5);
    innerlayout->addWidget(zoomin, 0, 0);
    innerlayout->addWidget(zoomout, 1, 0);
    innerlayout->addWidget(followgps, 2, 0);
    innerlayout->addWidget(createPath, 3, 0);
    // Add spacers to compress buttons on the top left
    innerlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 4, 0);
    innerlayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, 1, 0, 5);
    innerlayout->setRowStretch(0, 1);
    innerlayout->setRowStretch(1, 100);
    mc->setLayout(innerlayout);


    // Connect the required signals-slots
    connect(zoomin, SIGNAL(clicked(bool)),
            mc, SLOT(zoomIn()));

    connect(zoomout, SIGNAL(clicked(bool)),
            mc, SLOT(zoomOut()));

    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
            this, SLOT(addUAS(UASInterface*)));

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

    path = new LineString (wps, "UAV Path", pointPen);
    mc->layer("Waypoints")->addGeometry(path);

    this->setVisible(false);
}


void MapWidget::mapproviderSelected(QAction* action)
{
    //delete mapadapter;
    mapButton->setText(action->text());
    if (action == osmAction)
    {
        int zoom = mapadapter->adaptedZoom();
        mc->setZoom(0);

        mapadapter = new OSMMapAdapter();
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

        mapadapter = new YahooMapAdapter();
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

        mapadapter = new YahooMapAdapter("us.maps3.yimg.com", "/aerial.maps.yimg.com/png?v=1.7&t=a&s=256&x=%2&y=%3&z=%1");
        l->setMapAdapter(mapadapter);

        mc->updateRequestNew();
        mc->setZoom(zoom);
//        yahooActionOverlay->setEnabled(true);
    }
    else if (action == googleActionMap)
    {
        int zoom = mapadapter->adaptedZoom();
        mc->setZoom(0);
        mapadapter = new GoogleMapAdapter();
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
        mapadapter = new GoogleSatMapAdapter();
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
        emit createGlobalWP(true);

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


void MapWidget::captureMapClick(const QMouseEvent* event, const QPointF coordinate){

  if (QEvent::MouseButtonRelease == event->type() && createPath->isChecked()){
    // Create waypoint name
    QString str;

    str = QString("WP%1").arg(path->numberOfPoints());

    // create the WP and set everything in the LineString to display the path
    CirclePoint* tempCirclePoint = new CirclePoint(coordinate.x(), coordinate.y(), 10, str);
    mc->layer("Waypoints")->addGeometry(tempCirclePoint);

    Point* tempPoint = new Point(coordinate.x(), coordinate.y(),str);
    wps.append(tempPoint);
    path->addPoint(tempPoint);

    wpIndex.insert(str,tempPoint);

    // Refresh the screen
    mc->updateRequestNew();

    // emit signal mouse was clicked
    emit captureMapCoordinateClick(coordinate);

  }
}

void MapWidget::captureGeometryClick(Geometry* geom, QPoint point){
  Q_UNUSED(geom);
  Q_UNUSED(point);

  mc->setMouseMode(qmapcontrol::MapControl::None);


}

void MapWidget::captureGeometryDrag(Geometry* geom, QPointF coordinate){
  Q_UNUSED(coordinate);

  Point* point2Find;
  point2Find = wpIndex[geom->name()];
  point2Find->setCoordinate(coordinate);

  point2Find = dynamic_cast <Point*> (geom);
  point2Find->setCoordinate(coordinate);

  // Refresh the screen
  mc->updateRequestNew();
}

void MapWidget::captureGeometryEndDrag(Geometry* geom, QPointF coordinate){
  mc->setMouseMode(qmapcontrol::MapControl::Panning);

//  qDebug() << geom->name();
//  qDebug() << geom->GeometryType;
//  qDebug() << point;
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
    mav = uas;
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
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
    quint64 currTime = MG::TIME::getGroundTimeNow();
    if (currTime - lastUpdate > 90)
    {
        lastUpdate = currTime;
        // create a LineString
        //QList<Point*> points;
        // Points with a circle
        // A QPen can be used to customize the
        //pointpen->setWidth(3);
        //points.append(new CirclePoint(lat, lon, 10, uas->getUASName(), Point::Middle, pointpen));

        if (!uasIcons.contains(uas->getUASID()))
        {
            // Get the UAS color
            QColor uasColor = uas->getColor();

            // Icon
            QPen* pointpen = new QPen(uasColor);
            CirclePoint* p = new CirclePoint(lat, lon, 10, uas->getUASName(), Point::Middle, pointpen);
            uasIcons.insert(uas->getUASID(), p);
            geomLayer->addGeometry(p);

            // Line
            // A QPen also can use transparency

            QList<Point*> points;
            points.append(new Point(lat, lon, QString("lat: %1 lon: %2").arg(lat, lon)));
            QPen* linepen = new QPen(uasColor.darker());
            linepen->setWidth(2);
            // Add the Points and the QPen to a LineString
            LineString* ls = new LineString(points, uas->getUASName(), linepen);
            uasTrails.insert(uas->getUASID(), ls);

            // Add the LineString to the layer
            geomLayer->addGeometry(ls);
        }
        else
        {
            CirclePoint* p = uasIcons.value(uas->getUASID());
            p->setCoordinate(QPointF(lat, lon));
            // Extend trail
            uasTrails.value(uas->getUASID())->addPoint(new Point(lat, lon, QString("lat: %1 lon: %2").arg(lat, lon)));
        }

        //    points.append(new CirclePoint(8.275145, 50.016992, 15, "Wiesbaden-Mainz-Kastel, Johannes-Goßner-Straße", Point::Middle, pointpen));
        //    points.append(new CirclePoint(8.270476, 50.021426, 15, "Wiesbaden-Mainz-Kastel, Ruthof", Point::Middle, pointpen));
        //    // "Blind" Points
        //    points.append(new Point(8.266445, 50.025913, "Wiesbaden-Mainz-Kastel, Mudra Kaserne"));
        //    points.append(new Point(8.260378, 50.030345, "Wiesbaden-Mainz-Amoneburg, Dyckerhoffstraße"));

        // Connect click events of the layer to this object
        // connect(osmLayer, SIGNAL(geometryClicked(Geometry*, QPoint)),
        //                  this, SLOT(geometryClicked(Geometry*, QPoint)));

        // Sets the view to the interesting area
        //QList<QPointF> view;
        //view.append(QPointF(8.24764, 50.0319));
        //view.append(QPointF(8.28412, 49.9998));
        // mc->setView(view);
        updatePosition(0, lat, lon);
    }
}

/**
 * Center the view on this position
 */
void MapWidget::updatePosition(float time, double lat, double lon)
{
    Q_UNUSED(time);
    //gpsposition->setText(QString::number(time) + " / " + QString::number(lat) + " / " + QString::number(lon));
    if (followgps->isChecked())
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
void MapWidget::clearPath()
{
    // Clear the previous WP track

    mc->layer("Waypoints")->clearGeometries();
    wps.clear();
    path->setPoints(wps);
    mc->layer("Waypoints")->addGeometry(path);
    wpIndex.clear();
    mc->updateRequestNew();
}
