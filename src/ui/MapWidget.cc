/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of map view
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

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
    TileMapAdapter* osmAdapter = new TileMapAdapter("tile.openstreetmap.org", "/%1/%2/%3.png", 256, 0, 17);

    // create a layer with the mapadapter and type MapLayer
    osmLayer = new Layer("Custom Layer", osmAdapter, Layer::MapLayer);

    // add Layer to the MapControl
    mc->addLayer(osmLayer);
    mc->setZoom(3);

    // display the MapControl in the application
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mc);
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
    QVBoxLayout* innerlayout = new QVBoxLayout;
    innerlayout->addWidget(zoomin);
    innerlayout->addWidget(zoomout);
    innerlayout->addWidget(followgps);
    innerlayout->addWidget(createPath);
    mc->setLayout(innerlayout);


    // Connect the required signals-slots
    connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)),
            this, SLOT(addUAS(UASInterface*)));

    connect(mc, SIGNAL(mouseEventCoordinate(const QMouseEvent*, const QPointF)),
            this, SLOT(captureMapClick(const QMouseEvent*, const QPointF)));

    connect(createPath, SIGNAL(clicked(bool)),
            this, SLOT(createPathButtonClicked()));


    this->setVisible(false);


    // Attic (Code that was commented)
    // ==============================
    //uasIcons = QMap<int, CirclePoint*>();

    //QSize(480,640)
    //      ImageManager::instance()->setProxy("www-cache", 8080);

    //GoogleSatMapAdapter* gSatAdapter = new GoogleSatMapAdapter();
    //Layer* gSatLayer = new Layer("Custom Layer", gSatAdapter, Layer::MapLayer);
    //mc->addLayer(gSatLayer);

    // gpsposition = new QLabel();
    //gpsposition->setFont(QFont("Arial", 10));
    //GPS_Neo* gm = new GPS_Neo();
    //connect(gm, SIGNAL(new_position(float, QPointF)),
    //                  this, SLOT(updatePosition(float, QPointF)));
    //gm->start();

}

void MapWidget::createPathButtonClicked(){
  this->setCursor(createPath->isChecked()? Qt::PointingHandCursor : Qt::ArrowCursor);
}


void MapWidget::captureMapClick(const QMouseEvent* event, const QPointF coordinate){
  if (QEvent::MouseButtonRelease == event->type() && createPath->isChecked()){
    qDebug()<< "Click Event";
    qDebug()<< "Lat: " << coordinate.y();
    qDebug()<< "Lon: " << coordinate.x();
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
    mav = uas;
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
}

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
            osmLayer->addGeometry(p);

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
            osmLayer->addGeometry(ls);
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
