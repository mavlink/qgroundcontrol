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

class QMenu;

namespace Ui {
    class MapWidget;
}

using namespace qmapcontrol;

/**
 * @brief 2D Moving map
 *
 * This map displays street maps, aerial images and the MAV position,
 * waypoints and trails on top.
 */
class MapWidget : public QWidget {
    Q_OBJECT
public:
    MapWidget(QWidget *parent = 0);
    ~MapWidget();

public slots:
    void addUAS(UASInterface* uas);
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);
    void updatePosition(float time, double lat, double lon);

protected:
    void changeEvent(QEvent* e);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent* event);

    QAction* osmAction;
    QAction* yahooActionMap;
    QAction* yahooActionSatellite;
    QAction* yahooActionOverlay;
    QAction* googleActionMap;
    QAction* googleSatAction;


    QPushButton* followgps;
    QPushButton* createPath;
    QLabel* gpsposition;
    QMenu* mapMenu;
    QPushButton* mapButton;

    MapControl* mc;                   ///< QMapControl widget
    MapAdapter* mapadapter;           ///< Adapter to load the map data
    qmapcontrol::Layer* l;            ///< Current map layer (background)
    qmapcontrol::Layer* overlay;      ///< Street overlay (foreground)
    qmapcontrol::GeometryLayer* geomLayer; ///< Layer for waypoints
    int zoomLevel;
    int detailZoom; ///< Steps zoomed in further than qMapControl allows
    static const int scrollStep = 40; ///< Scroll n pixels per keypress
    static const int maxZoom = 50;    ///< Maximum zoom level

    //Layer* gSatLayer;

    QMap<int, CirclePoint*> uasIcons;
    QMap<int, LineString*> uasTrails;
    UASInterface* mav;
    quint64 lastUpdate;

  protected slots:
    void captureMapClick (const QMouseEvent* event, const QPointF coordinate);
    void createPathButtonClicked(bool checked);
    void captureGeometryClick(Geometry*, QPoint);
    void mapproviderSelected(QAction* action);
    void captureGeometryDrag(Geometry* geom, QPointF coordinate);
    void captureGeometryEndDrag(Geometry* geom, QPointF coordinate);

  signals:
    void movePoint(QPointF newCoord);

private:
    Ui::MapWidget *m_ui;
    QList<Point*> wps;
    QHash <QString, Point*> wpIndex;
    LineString* path;
    QPen* pointPen;
};

#endif // MAPWIDGET_H
