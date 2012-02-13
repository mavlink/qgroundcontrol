///*=====================================================================
//
//QGroundControl Open Source Ground Control Station
//
//(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
//
//This file is part of the QGROUNDCONTROL project
//
//    QGROUNDCONTROL is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    QGROUNDCONTROL is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
//
//======================================================================*/

/**
 * @file
 *   @brief Definition of the class Pixhawk3DWidget.
 *
 *   @author Lionel Heng <hengli@inf.ethz.ch>
 *
 */

#ifndef PIXHAWK3DWIDGET_H
#define PIXHAWK3DWIDGET_H

#include <osgText/Text>

#include "HUDScaleGeode.h"
#include "Imagery.h"
#include "ImageWindowGeode.h"
#include "WaypointGroupNode.h"
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    #include "ObstacleGroupNode.h"
#endif

#include "Q3DWidget.h"

class UASInterface;

/**
 * @brief A 3D View widget which displays vehicle-centric information.
 **/
class Pixhawk3DWidget : public Q3DWidget
{
    Q_OBJECT

public:
    explicit Pixhawk3DWidget(QWidget* parent = 0);
    ~Pixhawk3DWidget();

public slots:
    void setActiveUAS(UASInterface* uas);
    void addToTrails(UASInterface* uas, int component, double x, double y, double z, quint64 time);
    void updateAttitude(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 time);

private slots:
    void selectFrame(QString text);
    void showLocalGrid(int state);
    void showWorldGrid(int state);
    void showTrails(int state);
    void showWaypoints(int state);
    void selectMapSource(int index);
    void selectVehicleModel(int index);
    void recenter(void);
    void toggleFollowCamera(int state);

    void selectTargetHeading(void);
    void selectTarget(void);
    void setTarget(void);
    void insertWaypoint(void);
    void moveWaypointPosition(void);
    void moveWaypointHeading(void);
    void deleteWaypoint(void);
    void setWaypointAltitude(void);
    void clearAllWaypoints(void);

protected:
    QVector< osg::ref_ptr<osg::Node> > findVehicleModels(void);
    void buildLayout(void);
    virtual void resizeGL(int width, int height);
    virtual void display(void);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);

    UASInterface* uas;

signals:
    void visibilityChanged(bool visible);

private:
    void getPose(double& x, double& y, double& z,
                 double& roll, double& pitch, double& yaw,
                 QString& utmZone);
    void getPose(double& x, double& y, double& z,
                 double& roll, double& pitch, double& yaw);
    void getPosition(double& x, double& y, double& z,
                     QString& utmZone);
    void getPosition(double& x, double& y, double& z);

    osg::ref_ptr<osg::Geode> createLocalGrid(void);
    osg::ref_ptr<osg::Geode> createWorldGrid(void);
    osg::ref_ptr<osg::Geometry> createTrail(const osg::Vec4& color);
    osg::ref_ptr<Imagery> createMap(void);
    osg::ref_ptr<osg::Geode> createRGBD3D(void);
    osg::ref_ptr<osg::Node> createTarget(void);

    void setupHUD(void);
    void resizeHUD(void);

    void updateHUD(double robotX, double robotY, double robotZ,
                   double robotRoll, double robotPitch, double robotYaw,
                   const QString& utmZone);
    void updateTrails(double robotX, double robotY, double robotZ);
    void updateImagery(double originX, double originY, double originZ,
                       const QString& zone);
    void updateWaypoints(void);
    void updateTarget(double robotX, double robotY, double robotZ);
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    void updateRGBD(double robotX, double robotY, double robotZ);
    void updateObstacles(double robotX, double robotY, double robotZ);
    void updatePath(double robotX, double robotY, double robotZ);
#endif

    int findWaypoint(const QPoint& mousePos);
    bool findTarget(int mouseX, int mouseY);
    void showInsertWaypointMenu(const QPoint& cursorPos);
    void showEditWaypointMenu(const QPoint& cursorPos);

    const qreal kMessageTimeout; // message timeout in seconds

    enum Mode {
        DEFAULT_MODE,
        MOVE_WAYPOINT_POSITION_MODE,
        MOVE_WAYPOINT_HEADING_MODE,
        SELECT_TARGET_HEADING_MODE
    };
    Mode mode;
    int selectedWpIndex;

    bool displayLocalGrid;
    bool displayWorldGrid;
    bool displayTrails;
    bool displayImagery;
    bool displayWaypoints;
    bool displayRGBD2D;
    bool displayRGBD3D;
    bool displayObstacleList;
    bool displayPath;
    bool enableRGBDColor;
    bool enableTarget;

    bool followCamera;

    QMap<int, QVarLengthArray<osg::Vec3d, 10000> > trails;
    QMap<int, int> trailDrawableIdxs;

    osg::ref_ptr<osg::Node> vehicleModel;
    osg::ref_ptr<osg::Geometry> hudBackgroundGeometry;
    osg::ref_ptr<osgText::Text> statusText;
    osg::ref_ptr<HUDScaleGeode> scaleGeode;
    osg::ref_ptr<ImageWindowGeode> rgb2DGeode;
    osg::ref_ptr<ImageWindowGeode> depth2DGeode;
    osg::ref_ptr<osg::Image> rgbImage;
    osg::ref_ptr<osg::Image> depthImage;
    osg::ref_ptr<osg::Geode> localGridNode;
    osg::ref_ptr<osg::Geode> worldGridNode;
    osg::ref_ptr<osg::Geode> trailNode;
    osg::ref_ptr<osg::Group> orientationNode;
    osg::ref_ptr<Imagery> mapNode;
    osg::ref_ptr<WaypointGroupNode> waypointGroupNode;
    osg::ref_ptr<osg::Node> targetNode;
    osg::ref_ptr<osg::Geode> rgbd3DNode;
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    osg::ref_ptr<ObstacleGroupNode> obstacleGroupNode;
    osg::ref_ptr<osg::Geode> pathNode;
#endif

    QVector< osg::ref_ptr<osg::Node> > vehicleModels;

    MAV_FRAME frame;
    QVector4D target;
    QPoint cachedMousePos;
    double lastRobotX, lastRobotY, lastRobotZ;
};

#endif // PIXHAWK3DWIDGET_H
