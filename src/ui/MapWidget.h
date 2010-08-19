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
 *   @brief Definition of map view
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QtGui/QWidget>
#include <QMap>
#include "qmapcontrol.h"
#include "UASInterface.h"

namespace Ui {
    class MapWidget;
}

using namespace qmapcontrol;

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


    QPushButton* followgps;
    QPushButton* createPath;
    QLabel* gpsposition;

    MapControl* mc;
    int zoomLevel;
    int detailZoom; ///< Steps zoomed in further than qMapControl allows
    static const int scrollStep = 40; ///< Scroll n pixels per keypress
    static const int maxZoom = 50;
    TileMapAdapter* osmAdapter;
    GoogleSatMapAdapter* gSatAdapter;
    Layer* osmLayer;
    Layer* geomLayer;

    //Layer* gSatLayer;

    QMap<int, CirclePoint*> uasIcons;
    QMap<int, LineString*> uasTrails;
    UASInterface* mav;
    quint64 lastUpdate;

  protected slots:
    void captureMapClick (const QMouseEvent* event, const QPointF coordinate);
    void createPathButtonClicked();
    void captureGeometryClick(Geometry*, QPoint);
private:
    Ui::MapWidget *m_ui;
    QList<Point*> wps;
    LineString* path;
    QPen* pointPen;
};

#endif // MAPWIDGET_H
