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

MapWidget::MapWidget(QWidget *parent) :
        QWidget(parent),
        zoomLevel(0),
        m_ui(new Ui::MapWidget)
{
    m_ui->setupUi(this);
    // Accept focus by clicking or keyboard
    this->setFocusPolicy(Qt::StrongFocus);

    // create MapControl
    mc = new MapControl(QSize(320, 240));
    mc->showScale(true);
    mc->enablePersistentCache();

    //QSize(480,640)
    //      ImageManager::instance()->setProxy("www-cache", 8080);

    // create MapAdapter to get maps from
    TileMapAdapter* osmAdapter = new TileMapAdapter("tile.openstreetmap.org", "/%1/%2/%3.png", 256, 0, 17);
    //GoogleSatMapAdapter* gSatAdapter = new GoogleSatMapAdapter();

    // create a layer with the mapadapter and type MapLayer
    Layer* osmLayer = new Layer("Custom Layer", osmAdapter, Layer::MapLayer);
    //Layer* gSatLayer = new Layer("Custom Layer", gSatAdapter, Layer::MapLayer);

    // add Layer to the MapControl
    mc->addLayer(osmLayer);
    //mc->addLayer(gSatLayer);

    // display the MapControl in the application
    QHBoxLayout* layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mc);
    setLayout(layout);

    // create buttons as controls for zoom
    QPushButton* zoomin = new QPushButton(QIcon(":/images/actions/list-add.svg"), "", this);
    QPushButton* zoomout = new QPushButton(QIcon(":/images/actions/list-remove.svg"), "", this);
    followgps = new QPushButton(QIcon(":/images/actions/system-lock-screen.svg"), "", this);
    followgps->setCheckable(true);
    //gpsposition = new QLabel();
    zoomin->setMaximumWidth(50);
    zoomout->setMaximumWidth(50);
    followgps->setMaximumWidth(50);
    //gpsposition->setFont(QFont("Arial", 10));

    connect(zoomin, SIGNAL(clicked(bool)),
            mc, SLOT(zoomIn()));
    connect(zoomout, SIGNAL(clicked(bool)),
            mc, SLOT(zoomOut()));

    // add zoom buttons to the layout of the MapControl
    QVBoxLayout* innerlayout = new QVBoxLayout;
    innerlayout->addWidget(zoomin);
    innerlayout->addWidget(zoomout);
    innerlayout->addWidget(followgps);
    //innerlayout->addWidget(gpsposition);
    mc->setLayout(innerlayout);

    //GPS_Neo* gm = new GPS_Neo();
    //connect(gm, SIGNAL(new_position(float, QPointF)),
    //                  this, SLOT(updatePosition(float, QPointF)));
    //gm->start();

    mc->setZoom(3);
}

MapWidget::~MapWidget()
{
    delete m_ui;
}


void MapWidget::updatePosition(float time, QPointF coordinate)
{
    gpsposition->setText(QString::number(time) + " / " + QString::number(coordinate.x()) + " / " + QString::number(coordinate.y()));
    if (followgps->isChecked())
    {
        mc->setView(coordinate);
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
