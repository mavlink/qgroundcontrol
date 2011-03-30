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
 *   @brief Definition of MapWidget
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *   @author Mariano Lizarraga
 *
 */

#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QtGui/QWidget>
#include <QMap>
#include "qmapcontrol.h"
#include "UASInterface.h"
#include "QPointF"

#include <qmath.h>



class QMenu;
class Waypoint;
class Waypoint2DIcon;

namespace Ui
{
class MapWidget;
}

using namespace qmapcontrol;

/**
 * @brief 2D Moving map
 *
 * This map displays street maps, aerial images and the MAV position,
 * waypoints and trails on top.
 */
class MapWidget : public QWidget
{
    Q_OBJECT
public:
    MapWidget(QWidget *parent = 0);
    ~MapWidget();

public slots:
    void addUAS(UASInterface* uas);
    void activeUASSet(UASInterface* uas);
    /** @brief Update the system specs of one system */
    void updateSystemSpecs(int uas);
    /** @brief Update the selected system */
    void updateSelectedSystem(int uas);
    /** @brief Update the attitude */
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 usec);
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);
    void updatePosition(float time, double lat, double lon);
    void updateCameraPosition(double distance, double bearing, QString dir);
    QPointF getPointxBearing_Range(double lat1, double lon1, double bearing, double distance);

    /** @brief Clear the waypoints overlay layer */
    void clearWaypoints(int uas=0);
    /** @brief Clear the UAV tracks on the map */
    void clearPath(int uas=0);

    /** @brief Update waypoint list */
    void updateWaypointList(int uas);
    /** @brief Clear all waypoints and re-add them from updated list */
    void redoWaypoints(int uas=0);

    /** @brief Update waypoint */
    void updateWaypoint(int uas, Waypoint* wp);
    void updateWaypoint(int uas, Waypoint* wp, bool updateView);

    void drawBorderCamAtMap(bool status);
    /** @brief Bring up dialog to go to a specific location */
    void goTo();

protected:
    void changeEvent(QEvent* e);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent* event);
    /** @brief Start widget updating */
    void showEvent(QShowEvent* event);
    /** @brief Stop widget updating */
    void hideEvent(QHideEvent* event);

    QAction* osmAction;
    QAction* yahooActionMap;
    QAction* yahooActionSatellite;
    QAction* yahooActionOverlay;
    QAction* googleActionMap;
    QAction* googleSatAction;


    QPushButton* followgps;
    QPushButton* createPath;
    QPushButton* clearTracking;
    QLabel* gpsposition;
    QMenu* mapMenu;
    QPushButton* mapButton;

    qmapcontrol::MapControl* mc;            ///< QMapControl widget
    qmapcontrol::MapAdapter* mapadapter;    ///< Adapter to load the map data
    qmapcontrol::Layer* l;                  ///< Current map layer (background)
    qmapcontrol::Layer* overlay;            ///< Street overlay (foreground)
    qmapcontrol::Layer* tracks;             ///< Layer for UAV tracks
    qmapcontrol::GeometryLayer* geomLayer;  ///< Layer for waypoints

    //only for experiment
    qmapcontrol::GeometryLayer* camLayer; ///< Layer for camera indicator

    int zoomLevel;
    int detailZoom; ///< Steps zoomed in further than qMapControl allows
    static const int scrollStep = 40; ///< Scroll n pixels per keypress
    static const int maxZoom = 50;    ///< Maximum zoom level

    //Layer* gSatLayer;

    QMap<int, qmapcontrol::Point*> uasIcons;
    QMap<int, qmapcontrol::LineString*> uasTrails;
    QMap<int, QPen*> mavPens;
    //QMap<int, QList<qmapcontrol::Point*> > mavWps;
    //QMap<int, qmapcontrol::LineString*> waypointPaths;
    UASInterface* mav;
    quint64 lastUpdate;
    bool initialized;

    /** @brief Initialize view */
    void init();

protected slots:
    void captureMapClick (const QMouseEvent* event, const QPointF coordinate);
    void captureGeometryClick(Geometry*, QPoint);
    void captureGeometryDrag(Geometry* geom, QPointF coordinate);
    void captureGeometryEndDrag(Geometry* geom, QPointF coordinate);

    void createPathButtonClicked(bool checked);

    /** @brief Create the graphic representation of the waypoint */
    void createWaypointGraphAtMap(int id, const QPointF coordinate);
    void mapproviderSelected(QAction* action);

signals:
    //void movePoint(QPointF newCoord);
    //void captureMapCoordinateClick(const QPointF coordinate); //ROCA
    //void createGlobalWP(bool value, QPointF centerCoordinate);
    void waypointCreated(Waypoint* wp);
    void sendGeometryEndDrag(const QPointF coordinate, const int index);


private:
    Ui::MapWidget *m_ui;
    QList<qmapcontrol::Point*> wps;
    QList<Waypoint2DIcon*>wpIcons;
    qmapcontrol::LineString* waypointPath;
    //QHash <QString, qmapcontrol::Point*> wpIndex;
    QPen* pointPen;
    int wpExists(const QPointF coordinate);
    bool waypointIsDrag;


    qmapcontrol::LineString* camLine;
    QList<qmapcontrol::Point*> camPoints;
    QPointF lastCamBorderPos;
    bool drawCamBorder;
    int radioCamera;

};

#endif // MAPWIDGET_H
