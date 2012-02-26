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
#include "Q3DWidget.h"
#include "SystemContainer.h"
#include "ViewParamWidget.h"

class UASInterface;

/**
 * @brief A 3D View widget which displays vehicle-centric information.
 **/
class Pixhawk3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Pixhawk3DWidget(QWidget* parent = 0);
    ~Pixhawk3DWidget();

public slots:
    void activeSystemChanged(UASInterface* uas);
    void systemCreated(UASInterface* uas);
    void localPositionChanged(UASInterface* uas, int component, double x, double y, double z, quint64 time);
    void localPositionChanged(UASInterface* uas, double x, double y, double z, quint64 time);
    void attitudeChanged(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 time);
    void attitudeChanged(UASInterface* uas, double roll, double pitch, double yaw, quint64 time);
    void homePositionChanged(int uasId, double lat, double lon, double alt);
    void setpointChanged(int uasId, float x, float y, float z, float yaw);

signals:
    void systemCreatedSignal(UASInterface* uas);
    void overlayCreatedSignal(int systemId, const QString& name);

private slots:
    void clearData(void);
    void showTerrainParamWindow(void);
    void showViewParamWindow(void);
    void followCameraChanged(int systemId);
    void imageryParamsChanged(void);
    void recenterActiveCamera(void);
    void modelChanged(int systemId, int index);
    void setBirdEyeView(void);
    void loadTerrainModel(void);

#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    void addOverlay(UASInterface* uas);
#endif

    void selectTargetHeading(void);
    void selectTarget(void);
    void setTarget(void);
    void insertWaypoint(void);
    void moveWaypointPosition(void);
    void moveWaypointHeading(void);
    void deleteWaypoint(void);
    void setWaypointAltitude(void);
    void clearAllWaypoints(void);

    void moveImagery(void);
    void moveTerrain(void);
    void rotateTerrain(void);

    void sizeChanged(int width, int height);
    void update(void);

protected:
    void addModels(QVector< osg::ref_ptr<osg::Node> >& models,
                   const QColor& systemColor);
    void buildLayout(void);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);

    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

signals:
    void visibilityChanged(bool visible);

private:
    void initializeSystem(int systemId, const QColor& systemColor);

    void getPose(UASInterface* uas,
                 MAV_FRAME frame,
                 double& x, double& y, double& z,
                 double& roll, double& pitch, double& yaw,
                 QString& utmZone) const;
    void getPose(UASInterface* uas,
                 MAV_FRAME frame,
                 double& x, double& y, double& z,
                 double& roll, double& pitch, double& yaw) const;
    void getPosition(UASInterface* uas,
                     MAV_FRAME frame,
                     double& x, double& y, double& z,
                     QString& utmZone) const;
    void getPosition(UASInterface* uas,
                     MAV_FRAME frame,
                     double& x, double& y, double& z) const;

    osg::ref_ptr<osg::Geode> createLocalGrid(void);
    osg::ref_ptr<osg::Geode> createWorldGrid(void);
    osg::ref_ptr<osg::Geometry> createTrail(const osg::Vec4& color);
    osg::ref_ptr<osg::Geometry> createLink(const QColor& color);
    osg::ref_ptr<Imagery> createImagery(void);
    osg::ref_ptr<osg::Geode> createPointCloud(void);
    osg::ref_ptr<osg::Node> createTarget(const QColor& color);

    void setupHUD(void);
    void resizeHUD(int width, int height);

    void updateHUD(UASInterface* uas, MAV_FRAME frame);
    void updateImagery(double originX, double originY,
                       const QString& zone, MAV_FRAME frame);
    void updateTarget(UASInterface* uas, MAV_FRAME frame,
                      double robotX, double robotY, double robotZ,
                      QVector4D& target,
                      osg::ref_ptr<osg::Node>& targetNode);
    void updateTrails(double robotX, double robotY, double robotZ,
                      osg::ref_ptr<osg::Geode>& trailNode,
                      osg::ref_ptr<osg::Group>& orientationNode,
                      QMap<int, QVector<osg::Vec3d> >& trailMap,
                      QMap<int, int>& trailIndexMap);
    void updateWaypoints(UASInterface* uas, MAV_FRAME frame,
                         osg::ref_ptr<WaypointGroupNode>& waypointGroupNode);
#if defined(QGC_PROTOBUF_ENABLED) && defined(QGC_USE_PIXHAWK_MESSAGES)
    void updateRGBD(UASInterface* uas, MAV_FRAME frame,
                    osg::ref_ptr<ImageWindowGeode>& rgbImageNode,
                    osg::ref_ptr<ImageWindowGeode>& depthImageNode);
    void updatePointCloud(UASInterface* uas, MAV_FRAME frame,
                          double robotX, double robotY, double robotZ,
                          osg::ref_ptr<osg::Geode>& pointCloudNode,
                          bool colorPointCloudByDistance);
    void updateObstacles(UASInterface* uas, MAV_FRAME frame,
                         double robotX, double robotY, double robotZ,
                         osg::ref_ptr<ObstacleGroupNode>& obstacleGroupNode);
    void updatePlannedPath(UASInterface* uas, MAV_FRAME frame,
                           double robotX, double robotY, double robotZ,
                           osg::ref_ptr<osg::Geode>& plannedPathNode);
#endif

    int findWaypoint(const QPoint& mousePos);
    bool findTarget(int mouseX, int mouseY);
    bool findTerrain(const QPoint& mousePos);
    void showInsertWaypointMenu(const QPoint& cursorPos);
    void showEditWaypointMenu(const QPoint& cursorPos);
    void showTerrainMenu(const QPoint& cursorPos);

    const qreal kMessageTimeout; // message timeout in seconds

    enum Mode {
        DEFAULT_MODE,
        MOVE_WAYPOINT_POSITION_MODE,
        MOVE_WAYPOINT_HEADING_MODE,
        SELECT_TARGET_HEADING_MODE,
        MOVE_TERRAIN_MODE,
        ROTATE_TERRAIN_MODE,
        MOVE_IMAGERY_MODE
    };
    Mode mMode;
    int mSelectedWpIndex;

    int mActiveSystemId;
    UASInterface* mActiveUAS;

    GlobalViewParamsPtr mGlobalViewParams;

    // maps system id to system-specific view parameters
    QMap<int, SystemViewParamsPtr> mSystemViewParamMap;

    // maps system id to system-specific data
    QMap<int, SystemContainer> mSystemContainerMap;

    osg::ref_ptr<osg::Geometry> mHudBackgroundGeometry;
    osg::ref_ptr<Imagery> mImageryNode;
    osg::ref_ptr<HUDScaleGeode> mScaleGeode;
    osg::ref_ptr<osgText::Text> mStatusText;
    osg::ref_ptr<osg::Node> mTerrainNode;
    osg::ref_ptr<osg::PositionAttitudeTransform> mTerrainPAT;
    osg::ref_ptr<osg::Geode> mWorldGridNode;

    QPoint mCachedMousePos;
    int mFollowCameraId;
    QVector3D mCameraPos;
    bool mInitCameraPos;

    Q3DWidget* m3DWidget;
    ViewParamWidget* mViewParamWidget;
};

#endif // PIXHAWK3DWIDGET_H
