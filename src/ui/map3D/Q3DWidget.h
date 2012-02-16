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
 *   @brief Definition of the class Q3DWidget.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#ifndef Q3DWIDGET_H
#define Q3DWIDGET_H

#include <QtOpenGL>
#include <inttypes.h>

#include <osg/LineSegment>
#include <osg/PositionAttitudeTransform>
#include <osgGA/TrackballManipulator>
#include <osgText/Font>
#include <osgViewer/Viewer>

#include "CameraParams.h"
#include "GCManipulator.h"
#include "SystemGroupNode.h"

/**
 * @brief Definition of the class Q3DWidget.
 * The Q3DWidget widget uses the OpenSceneGraph framework to render
 * user-defined objects in 3D. The world coordinate system in OSG is defined:
 * - x-axis points to the east
 * - y-axis points to the north
 * - z-axis points upwards
 */
class Q3DWidget : public QGLWidget, public osgViewer::Viewer
{
    Q_OBJECT

public:
    /**
     * Constructor.
     * @param parent Parent widget.
     */
    Q3DWidget(QWidget* parent = 0);

    /**
     * Destructor.
     */
    virtual ~Q3DWidget();

    /**
     * @brief Initializes the widget.
     * @param fps Frames per second.
     */
    void init(float fps);

    /**
     * @brief Sets the camera parameters.
     * @param minZoomRange Minimum distance from the viewer to the camera.
     * @param cameraFov Camera field of view.
     * @param minClipRange Distance from the viewer to the near clipping range.
     * @param maxClipRange Distance from the viewer to the far clipping range.
     */
    void setCameraParams(float minZoomRange, float cameraFov,
                         float minClipRange, float maxClipRange);

    /**
     * @brief Moves the camera by [dx,dy,dz].
     * @param dx Translation along the x-axis in meters.
     * @param dy Translation along the y-axis in meters.
     * @param dz Translation along the z-axis in meters.
     */
    void moveCamera(double dx, double dy, double dz);

    /**
     * @brief Recenters the camera at (x,y,z).
     */
    void recenterCamera(double x, double y, double z);

    /**
      * @brief Rotates the camera.
      */
    void rotateCamera(double roll, double pitch, double yaw);

    /**
      * @brief Sets the camera distance.
      */
    void setCameraDistance(double distance);

    /**
     * @brief Sets up 3D display mode.
     */
    void setDisplayMode3D(void);

    osg::ref_ptr<osg::Switch>& hudGroup(void);

    /**
     * @brief Get screen coordinates of mouse cursor.
     * @return screen coordinates of mouse cursor.
     */
    QPoint mouseCursorCoords(void);

    /**
     * @brief Gets the world 3D coordinates of the cursor.
     * The function projects the 2D cursor position to a line in world space
     * and returns the intersection of that line and the horizontal plane
     * which contains the point (0, 0, z);
     * @param cursorX x-coordinate of the cursor.
     * @param cursorY y-coordinate of the cursor.
     * @param worldZ z-coordinate of the point in the plane.
     * @return World (x,y) cursor coordinates.
     */
     QPointF worldCursorPosition(const QPoint& cursorPos, double worldZ) const;

     CameraParams& cameraParams(void);

     osg::ref_ptr<GCManipulator>& cameraManipulator(void);

     osg::ref_ptr<osgText::Font>& font(void);

     osg::ref_ptr<osg::Switch>& worldMap(void);
     osg::ref_ptr<SystemGroupNode>& systemGroup(int systemId);

     bool& handleDeviceEvents(void);

     void handleKeyPressEvent(QKeyEvent* event);

     void handleKeyReleaseEvent(QKeyEvent* event);

     void handleMousePressEvent(QMouseEvent* event);

     void handleMouseReleaseEvent(QMouseEvent* event);

     void handleMouseMoveEvent(QMouseEvent* event);

     void handleWheelEvent(QWheelEvent* event);

protected slots:
    /**
     * @brief Updates the widget.
     */
    void redraw(void);

signals:
    void sizeChanged(int width, int height);
    void update(void);

protected:
    /**
     * @brief Handle widget resize event.
     * @param width New width of widget.
     * @param height New height of widget.
     */
    virtual void resizeGL(int width, int height);

    /**
     * @brief Processes key press events.
     * @param event Key press event.
     */
    virtual void keyPressEvent(QKeyEvent* event);

    /**
     * @brief Processes key release events.
     * @param event Key release event.
     */
    virtual void keyReleaseEvent(QKeyEvent* event);

    /**
     * @brief Processes mouse press events.
     * @param event Mouse press event.
     */
    virtual void mousePressEvent(QMouseEvent* event);

    /**
     * @brief Processes mouse release events.
     * @param event Mouse release event.
     */
    virtual void mouseReleaseEvent(QMouseEvent* event);

    /**
     * @brief Processes mouse move events.
     * @param event Mouse move event.
     */
    virtual void mouseMoveEvent(QMouseEvent* event);

    /**
     * @brief Processes mouse wheel events.
     * @param event Mouse wheel event.
     */
    virtual void wheelEvent(QWheelEvent* event);

    /** @brief Start widget updating */
    void showEvent(QShowEvent* event);
    /** @brief Stop widget updating */
    void hideEvent(QHideEvent* event);

    /**
     * @brief Get base HUD geode.
     * @return Smart pointer to the geode.
     */
    osg::ref_ptr<osg::Node> createHUD(void);

    /**
     * @brief Handle widget paint event.
     */
    virtual void paintGL(void);

    /**
     * @brief Converts Qt-defined key to OSG-defined key.
     * @param key Qt-defined key.
     * @return OSG-defined key.
     */
    osgGA::GUIEventAdapter::KeySymbol convertKey(int key) const;

    /**
     * @brief Computes intersection of line and plane.
     * @param plane Vector which represents the plane.
     * @param line Line representation.
     * @return 3D point which lies at the intersection of the line and plane.
     */
    bool planeLineIntersection(const osg::Vec4d& plane,
                               const osg::LineSegment& line,
                               osg::Vec3d& isect) const;

    bool mHandleDeviceEvents;

    osg::ref_ptr<osg::Group> mRoot; /**< Root node of scene graph. */
    osg::ref_ptr<osg::Switch> mWorldMap;
    QMap<int, osg::ref_ptr<SystemGroupNode> > mSystemGroups;

    osg::ref_ptr<osg::Switch> mHudGroup; /**< A group which contains renderable HUD objects. */
    osg::ref_ptr<osg::Projection> mHudProjectionMatrix; /**< An orthographic projection matrix for HUD display. */

    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> mOsgGW; /**< A class which manages OSG graphics windows and events. */

    osg::ref_ptr<GCManipulator> mCameraManipulator; /**< Camera manipulator. */

    QTimer mTimer; /**< Timer which draws graphics based on specified fps. */

    CameraParams mCameraParams; /**< Struct representing camera parameters. */
    float mFps;

    osg::ref_ptr<osgText::Font> mFont;
};

#endif // Q3DWIDGET_H
